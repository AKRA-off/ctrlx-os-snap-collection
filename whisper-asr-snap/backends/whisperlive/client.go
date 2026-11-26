// Package whisperlive is a small client for a WhisperLive websocket backend.
//
// The flow is intentionally linear:
//
//	client, _ := Dial(ctx, cfg)   // connect + send config
//	client.WaitReady(ctx)         // block until the server says SERVER_READY
//	client.TranscribeFile(ctx, f) // stream audio, then finalize
//	text := client.FinalTranscript()
//	client.Close()
package whisperlive

import (
	"context"
	"encoding/binary"
	"encoding/json"
	"fmt"
	"log"
	"math"
	"myna-adapter/backends"
	"net/url"
	"os"
	"slices"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/gorilla/websocket"
)

const endOfAudioMarker = "END_OF_AUDIO"

// Segment is a single transcript segment reported by the backend.
type Segment struct {
	Start     float64
	End       float64
	Text      string
	Completed bool
}

// Config controls how the client connects and streams audio. Zero values are
// replaced with sensible defaults by withDefaults.
type Config struct {
	Host   string
	Port   int
	UseWSS bool

	Lang      string
	Model     string
	Translate bool
	UseVAD    bool

	// Streaming behavior.
	SampleRate       int           // audio sample rate in Hz (default 16000)
	ChunkBytes       int           // s16le bytes streamed per packet (default 4096)
	IdleTimeout      time.Duration // silence before sending END_OF_AUDIO (default 15s)
	ServerCloseGrace time.Duration // wait for the final flush after END_OF_AUDIO (default 5s)

	// Observability.
	LogTranscription bool
	OnTranscription  func(text string, segments []Segment)

	Callbacks        backends.BackendCallbacks
	AllowedModels    []string
	AllowedLanguages []string
}

func (c Config) withDefaults() Config {
	if c.SampleRate <= 0 {
		c.SampleRate = 16000
	}
	if c.ChunkBytes <= 0 {
		c.ChunkBytes = 4096
	}
	if c.IdleTimeout <= 0 {
		c.IdleTimeout = 15 * time.Second
	}
	if c.ServerCloseGrace <= 0 {
		c.ServerCloseGrace = 5 * time.Second
	}
	if c.Model == "" {
		c.Model = "small"
	}
	return c
}

// Client is a connected WhisperLive websocket session.
type Client struct {
	cfg  Config
	uid  string
	conn *websocket.Conn

	ready     chan struct{} // closed once the server is ready
	readyOnce sync.Once
	closed    chan struct{} // closed once the read loop exits
	closeOnce sync.Once
	done      chan struct{} // closed once the client is permanently shut down (not during a reload)
	doneOnce  sync.Once

	reloading bool // true while reloadBackend is swapping the connection

	writeMu sync.Mutex // serializes websocket writes

	mu           sync.Mutex
	readErr      error
	serverErr    string
	recording    bool
	transcript   []Segment // completed segments, in order
	partial      *Segment  // trailing, not-yet-completed segment
	lastText     string    // last segment text, used to detect activity
	lastLogged   string    // last line printed, used to dedupe logs
	lastActivity time.Time

	// Delta/commit tracking state (under mu).
	segmentID          int
	emitted            string
	emittedIsCommitted bool
	totalAudioDuration time.Duration
	audioFinalized     bool
}

// Dial connects to the backend, sends the initial configuration and starts the
// background read loop. It returns as soon as the socket is open; callers should
// use WaitReady to block until the backend is ready to receive audio.
func Dial(ctx context.Context, cfg Config) (*Client, error) {
	cfg = cfg.withDefaults()
	if strings.TrimSpace(cfg.Host) == "" || cfg.Port <= 0 {
		return nil, fmt.Errorf("host and port are required")
	}

	scheme := "ws"
	if cfg.UseWSS {
		scheme = "wss"
	}
	u := url.URL{Scheme: scheme, Host: fmt.Sprintf("%s:%d", cfg.Host, cfg.Port)}

	conn, _, err := websocket.DefaultDialer.DialContext(ctx, u.String(), nil)
	if err != nil {
		return nil, fmt.Errorf("dial websocket: %w", err)
	}

	c := &Client{
		cfg:          cfg,
		uid:          fmt.Sprintf("%d-%d", time.Now().UnixNano(), os.Getpid()),
		conn:         conn,
		ready:        make(chan struct{}),
		closed:       make(chan struct{}),
		done:         make(chan struct{}),
		lastActivity: time.Now(),
	}

	if err := c.sendConfig(); err != nil {
		_ = conn.Close()
		return nil, fmt.Errorf("send config: %w", err)
	}

	log.Printf("[INFO]: Opened connection to %s", u.String())
	go c.readLoop(c.closed)
	return c, nil
}

// WaitReady blocks until the backend reports SERVER_READY, the connection
// closes, or the context is cancelled.
func (c *Client) WaitReady(ctx context.Context) error {
	select {
	case <-ctx.Done():
		return ctx.Err()
	case <-c.ready:
		return nil
	case <-c.closed:
		if msg := c.errorMessage(); msg != "" {
			return fmt.Errorf("server error: %s", msg)
		}
		return fmt.Errorf("connection closed before server was ready")
	}
}

// Close tears down the websocket connection permanently.
func (c *Client) Close() error {
	c.doneOnce.Do(func() { close(c.done) })
	_ = c.conn.WriteControl(
		websocket.CloseMessage,
		websocket.FormatCloseMessage(websocket.CloseNormalClosure, ""),
		time.Now().Add(500*time.Millisecond),
	)
	return c.conn.Close()
}

// Done returns a channel that is closed when the client is permanently shut
// down. It is NOT closed during a reload — only when Close is called or the
// backend connection drops unexpectedly.
func (c *Client) Done() <-chan struct{} {
	return c.done
}

// FinalTranscript returns the accumulated completed segments plus the trailing
// partial segment, joined into a single string.
func (c *Client) FinalTranscript() string {
	c.mu.Lock()
	defer c.mu.Unlock()
	return c.joinLocked()
}

// sendConfig sends the initial JSON configuration message.
func (c *Client) sendConfig() error {
	task := "transcribe"
	if c.cfg.Translate {
		task = "translate"
	}
	language := c.cfg.Lang
	if language == "auto" {
		language = ""
	}
	payload := map[string]any{
		"uid":      c.uid,
		"language": nilIfEmpty(language),
		"task":     task,
		"model":    c.cfg.Model,
		"use_vad":  c.cfg.UseVAD,
	}
	msg, err := json.Marshal(payload)
	if err != nil {
		return err
	}
	return c.write(websocket.TextMessage, msg)
}

// readLoop reads and dispatches server messages until the connection ends.
// The closed channel must be the per-session channel that was current when this
// goroutine was launched; it must not be read back from c.closed so that a
// concurrent reloadBackend swap cannot redirect the close to the new session.
func (c *Client) readLoop(closed chan struct{}) {
	defer func() {
		close(closed)
		// Only signal permanent closure when this is not a planned reload.
		c.mu.Lock()
		reloading := c.reloading
		c.mu.Unlock()
		if !reloading {
			c.doneOnce.Do(func() { close(c.done) })
		}
	}()

	for {
		msgType, payload, err := c.conn.ReadMessage()
		if err != nil {
			c.mu.Lock()
			c.readErr = err
			c.recording = false
			c.mu.Unlock()
			log.Printf("[INFO]: Connection closed: %v", err)
			return
		}
		if msgType != websocket.TextMessage {
			continue
		}
		c.handleMessage(payload)
	}
}

func (c *Client) handleMessage(raw []byte) {
	var msg map[string]any
	if err := json.Unmarshal(raw, &msg); err != nil {
		log.Printf("[WARN]: invalid json from server: %v", err)
		return
	}

	if uid, _ := msg["uid"].(string); uid != c.uid {
		return
	}

	if status, ok := msg["status"].(string); ok {
		switch status {
		case "ERROR":
			text, _ := msg["message"].(string)
			c.mu.Lock()
			c.serverErr = text
			c.mu.Unlock()
			log.Printf("[ERROR]: %s", text)
		case "WARNING":
			text, _ := msg["message"].(string)
			log.Printf("[WARN]: %s", text)
		case "WAIT":
			log.Printf("[INFO]: Server is full, waiting for a slot")
		}
		return
	}

	if message, _ := msg["message"].(string); message == "SERVER_READY" {
		backend, _ := msg["backend"].(string)
		c.mu.Lock()
		c.recording = true
		c.lastActivity = time.Now()
		c.mu.Unlock()
		log.Printf("[INFO]: Server ready (backend: %s)", backend)
		c.readyOnce.Do(func() { close(c.ready) })
		if c.cfg.Callbacks.OnModelLoaded != nil {
			c.cfg.Callbacks.OnModelLoaded()
		}
		return
	}

	if message, _ := msg["message"].(string); message == "DISCONNECT" {
		c.mu.Lock()
		c.recording = false
		c.mu.Unlock()
		log.Printf("[INFO]: Server disconnected the session")
		if c.cfg.Callbacks.OnModelUnloaded != nil {
			c.cfg.Callbacks.OnModelUnloaded()
		}
		return
	}

	if lang, ok := msg["language"].(string); ok {
		log.Printf("[INFO]: Detected language %s", lang)
		return
	}

	if raw, ok := msg["segments"]; ok {
		c.updateSegments(parseSegments(raw))
	}
}

// updateSegments merges a fresh batch of segments into the running transcript.
func (c *Client) updateSegments(segments []Segment) {
	if len(segments) == 0 {
		return
	}

	c.mu.Lock()
	for _, seg := range segments {
		if !seg.Completed {
			continue
		}
		// Append only segments that start after the last completed one to avoid
		// re-appending the repeated window the backend keeps resending.
		if len(c.transcript) > 0 {
			lastSegment := c.transcript[len(c.transcript)-1]
			if seg.Start >= lastSegment.End && seg.Text != lastSegment.Text {
				c.transcript = append(c.transcript, seg)
			}
		} else {
			c.transcript = append(c.transcript, seg)
		}
	}

	last := segments[len(segments)-1]
	if last.Completed {
		c.partial = nil
	} else {
		cp := last
		c.partial = &cp
	}

	// Refresh the activity timestamp only when the tail text actually changes,
	// so the idle timer measures real silence, not repeated frames.
	if last.Text != c.lastText {
		c.lastActivity = time.Now()
		c.lastText = last.Text
	}

	line := c.joinLocked()
	changed := line != "" && line != c.lastLogged
	if changed {
		c.lastLogged = line
	}
	callback := c.cfg.OnTranscription
	logIt := c.cfg.LogTranscription && changed

	// Compute delta/commit callbacks to fire after releasing the lock.
	newSegmentID := len(segments) - 1
	var pending []func()

	if c.segmentID != newSegmentID {
		// The backend started a new segment; commit the previous one only if it
		// was not already committed
		if c.segmentID < len(segments) && !c.emittedIsCommitted {
			completedText := segments[c.segmentID].Text
			if fn := c.cfg.Callbacks.OnCommit; fn != nil {
				pending = append(pending, func() { fn(completedText) })
			}
		}
		c.emitted = ""
		c.emittedIsCommitted = false
		c.segmentID = newSegmentID
	}

	isUnsentDelta := !last.Completed && last.Text != c.emitted
	isUnsentCommit := last.Completed && !c.emittedIsCommitted
	isRepeatedUncommittedDeltaAtEndOfAudio := last.Text == c.emitted && !c.emittedIsCommitted && last.End == c.totalAudioDuration.Seconds() && c.audioFinalized

	if isUnsentDelta { // new partial text
		c.emittedIsCommitted = false
		if newText, ok := strings.CutPrefix(last.Text, c.emitted); ok && newText != "" {
			// Extend: only the new suffix is novel.
			c.emitted = last.Text
			if fn := c.cfg.Callbacks.OnDelta; fn != nil {
				pending = append(pending, func() { fn(newText) })
			}
		} else {
			// Revision: the backend rewrote the partial; reset and resend.
			c.emitted = last.Text
			if fn := c.cfg.Callbacks.OnCommit; fn != nil {
				pending = append(pending, func() { fn("") })
			}
			if fn := c.cfg.Callbacks.OnDelta; fn != nil {
				t := c.emitted
				pending = append(pending, func() { fn(t) })
			}
		}
	} else if isUnsentCommit || isRepeatedUncommittedDeltaAtEndOfAudio {
		c.emittedIsCommitted = true
		if fn := c.cfg.Callbacks.OnCommit; fn != nil {
			pending = append(pending, func() { fn(last.Text) })
		}
	}

	c.mu.Unlock()

	for _, cb := range pending {
		cb()
	}
	if callback != nil {
		callback(line, segments)
	}
	if logIt {
		log.Printf("[TRANSCRIPT] %s", line)
	}
}

// joinLocked builds the full transcript string. Callers must hold c.mu.
func (c *Client) joinLocked() string {
	parts := make([]string, 0, len(c.transcript)+1)
	for _, seg := range c.transcript {
		if text := strings.TrimSpace(seg.Text); text != "" {
			parts = append(parts, text)
		}
	}
	if c.partial != nil {
		if text := strings.TrimSpace(c.partial.Text); text != "" {
			if len(parts) == 0 || parts[len(parts)-1] != text {
				parts = append(parts, text)
			}
		}
	}
	return strings.TrimSpace(strings.Join(parts, " "))
}

// sendAudio streams a batch of PCM float32 samples to the backend.
func (c *Client) sendAudio(samples []float32) error {
	if len(samples) == 0 {
		return nil
	}
	buf := make([]byte, len(samples)*4)
	for i, s := range samples {
		binary.LittleEndian.PutUint32(buf[i*4:], math.Float32bits(s))
	}
	c.totalAudioDuration += time.Duration(len(samples)) * time.Second / time.Duration(c.cfg.SampleRate)
	return c.write(websocket.BinaryMessage, buf)
}

// SendPCM16 forwards a chunk of little-endian signed 16-bit mono PCM audio to
// the backend, converting it to the float32 format WhisperLive expects. It is
// intended for live streaming, where audio already arrives paced in real time,
// so it performs no pacing of its own. Safe for concurrent use.
//
// lastActivity is updated on every call so that waitUntilIdle (used by
// Finalize) measures idle time from the last audio packet, not from the last
// transcription update. Without this, a model that is slow to emit intermediate
// segments would have an idleFor() that already exceeds IdleTimeout by the time
// the commit arrives, causing END_OF_AUDIO to be sent before the backend has
// processed the audio.
func (c *Client) SendPCM16(pcm []byte) error {
	c.mu.Lock()
	c.lastActivity = time.Now()
	c.mu.Unlock()
	return c.sendAudio(pcm16ToFloat32(pcm))
}

func (c *Client) Finalize(ctx context.Context) error {
	c.mu.Lock()
	c.audioFinalized = true
	c.mu.Unlock()
	c.waitUntilIdle(ctx)
	_ = c.sendEndOfAudio()
	c.waitForClose(ctx)
	return nil
}

func (c *Client) SendEndOfAudio() error {
	return c.sendEndOfAudio()
}

func (c *Client) sendEndOfAudio() error {
	return c.write(websocket.BinaryMessage, []byte(endOfAudioMarker))
}

func (c *Client) write(messageType int, payload []byte) error {
	c.writeMu.Lock()
	defer c.writeMu.Unlock()
	return c.conn.WriteMessage(messageType, payload)
}

func (c *Client) recordingActive() bool {
	c.mu.Lock()
	defer c.mu.Unlock()
	return c.recording
}

func (c *Client) idleFor() time.Duration {
	c.mu.Lock()
	defer c.mu.Unlock()
	return time.Since(c.lastActivity)
}

func (c *Client) errorMessage() string {
	c.mu.Lock()
	defer c.mu.Unlock()
	return c.serverErr
}

func parseSegments(raw any) []Segment {
	list, ok := raw.([]any)
	if !ok {
		return nil
	}
	segments := make([]Segment, 0, len(list))
	for _, item := range list {
		m, ok := item.(map[string]any)
		if !ok {
			continue
		}
		start, _ := asFloat64(m["start"])
		end, _ := asFloat64(m["end"])
		text, _ := m["text"].(string)
		completed, _ := m["completed"].(bool)
		segments = append(segments, Segment{Start: start, End: end, Text: text, Completed: completed})
	}
	return segments
}

func asFloat64(v any) (float64, bool) {
	switch t := v.(type) {
	case float64:
		return t, true
	case float32:
		return float64(t), true
	case int:
		return float64(t), true
	case int64:
		return float64(t), true
	case json.Number:
		n, err := t.Float64()
		return n, err == nil
	case string:
		f, err := strconv.ParseFloat(t, 64)
		return f, err == nil
	}
	return 0, false
}

func nilIfEmpty(s string) any {
	if strings.TrimSpace(s) == "" {
		return nil
	}
	return s
}

func (c *Client) GetConfig() *backends.SessionConfig {
	c.mu.Lock()
	defer c.mu.Unlock()

	return &backends.SessionConfig{
		Type:         "realtime",
		Instructions: nil,
		Prompt:       nil,
		Include:      nil,
		Model:        c.cfg.Model,
		Lang:         c.cfg.Lang,
		AudioFormat:  "pcm16",
		SampleRate:   c.cfg.SampleRate,
	}
}

func (c *Client) SetConfig(cfg backends.SessionConfig) error {
	valid, err := c.ValidateSessionConfig(cfg)
	if !valid {
		return fmt.Errorf("invalid session config: %w", err)
	}

	c.mu.Lock()
	c.cfg.Model = cfg.Model
	c.cfg.Lang = cfg.Lang
	c.cfg.SampleRate = cfg.SampleRate
	c.mu.Unlock()

	return c.reloadBackend()
}

func (c *Client) ValidateSessionConfig(cfg backends.SessionConfig) (bool, error) {
	if cfg.SampleRate != 16000 {
		return false, fmt.Errorf("unsupported sample rate: %d", cfg.SampleRate)
	}
	if cfg.AudioFormat != "pcm16" {
		return false, fmt.Errorf("unsupported audio format: %s", cfg.AudioFormat)
	}
	if cfg.Type != "realtime" {
		return false, fmt.Errorf("unsupported session type: %s", cfg.Type)
	}
	if cfg.Prompt != nil {
		return false, fmt.Errorf("prompt is not supported")
	}
	if cfg.Instructions != nil {
		return false, fmt.Errorf("instructions are not supported")
	}

	supportedIncludes := []string{}

	for _, include := range cfg.Include {
		if !slices.Contains(supportedIncludes, include) {
			return false, fmt.Errorf("unsupported include: %s", include)
		}
	}

	if !slices.Contains(c.cfg.AllowedModels, cfg.Model) {
		return false, fmt.Errorf("model %q is not in allowed models: %v", cfg.Model, c.cfg.AllowedModels)
	}
	if !slices.Contains(c.cfg.AllowedLanguages, cfg.Lang) {
		return false, fmt.Errorf("language %q is not in allowed languages: %v", cfg.Lang, c.cfg.AllowedLanguages)
	}

	return true, nil
}

func (c *Client) reloadBackend() error {
	// Notify that the current model is being unloaded.
	if c.cfg.Callbacks.OnModelUnloaded != nil {
		c.cfg.Callbacks.OnModelUnloaded()
	}

	c.mu.Lock()
	cfg := c.cfg
	oldConn := c.conn
	oldClosed := c.closed

	// Signal that the upcoming read-loop exit is intentional so it does not
	// close the permanent done channel.
	c.reloading = true

	// Swap in fresh per-session channels and reset transcript state.
	c.closed = make(chan struct{})
	c.closeOnce = sync.Once{}
	c.ready = make(chan struct{})
	c.readyOnce = sync.Once{}
	c.transcript = nil
	c.partial = nil
	c.lastText = ""
	c.lastLogged = ""
	c.lastActivity = time.Now()
	c.segmentID = 0
	c.emitted = ""
	c.emittedIsCommitted = false
	c.recording = false
	c.mu.Unlock()

	// Close the old connection and wait for its read loop to exit so there is
	// no race between the old goroutine and the new one over c.conn.
	_ = oldConn.WriteControl(
		websocket.CloseMessage,
		websocket.FormatCloseMessage(websocket.CloseNormalClosure, ""),
		time.Now().Add(500*time.Millisecond),
	)
	_ = oldConn.Close()
	<-oldClosed

	// Dial the new connection.
	scheme := "ws"
	if cfg.UseWSS {
		scheme = "wss"
	}
	u := url.URL{Scheme: scheme, Host: fmt.Sprintf("%s:%d", cfg.Host, cfg.Port)}
	conn, _, err := websocket.DefaultDialer.DialContext(context.Background(), u.String(), nil)
	if err != nil {
		return fmt.Errorf("dial websocket: %w", err)
	}

	c.mu.Lock()
	c.conn = conn
	c.uid = fmt.Sprintf("%d-%d", time.Now().UnixNano(), os.Getpid())
	c.reloading = false
	c.mu.Unlock()

	if err := c.sendConfig(); err != nil {
		_ = conn.Close()
		return fmt.Errorf("send config: %w", err)
	}

	log.Printf("[INFO]: Reopened connection to %s", u.String())
	go c.readLoop(c.closed)
	return nil
}
