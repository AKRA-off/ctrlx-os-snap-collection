package server

import (
	"context"
	"encoding/base64"
	"fmt"
	"log"
	"sync"

	"myna-adapter/backends"
	"myna-adapter/openai/events"

	"github.com/gorilla/websocket"
)

// Session holds the per-connection state for a single Myna user. Each session
// owns a dedicated backend session: audio frames received from the user are
// forwarded to the backend, and transcription results are translated into
// OpenAI events and pushed to the user.
type Session struct {
	Connection *websocket.Conn

	factory backends.Factory
	backend backends.Backend
	ctx     context.Context // root context for the session lifetime

	writeMu sync.Mutex // serializes websocket writes to the user

	closeOnce sync.Once

	mu                   sync.Mutex
	modelLoaded          bool // true if the backend has loaded a model
	sessionStarted       bool // true if the user has sent any audio chunks
	audioBufferCommitted bool // true if the user has sent InputAudioBufferCommit
}

// NewSession creates a Session bound to a user websocket connection.
func NewSession(conn *websocket.Conn, factory backends.Factory) *Session {
	return &Session{Connection: conn, factory: factory}
}

// Start opens the backend session, waits until it is ready, and advertises the
// session configuration to the user via session.created.
func (s *Session) Start(ctx context.Context) error {
	s.ctx = ctx

	callbacks := backends.BackendCallbacks{
		OnDelta:         s.onDelta,
		OnCommit:        s.onCommit,
		OnModelLoaded:   s.onModelLoaded,
		OnModelUnloaded: s.onModelUnloaded,
	}

	backend, err := s.factory(ctx, callbacks)
	if err != nil {
		return fmt.Errorf("opening backend session: %w", err)
	}
	s.backend = backend

	if err := backend.WaitReady(ctx); err != nil {
		return fmt.Errorf("waiting for backend: %w", err)
	}

	created := events.NewSessionCreated(s.backend.GetConfig())

	if err := s.send(created); err != nil {
		return fmt.Errorf("sending session.created: %w", err)
	}

	// If the backend session ends (Myna closed the connection, errored,
	// or we tore it down), close the user connection too.
	go s.watchBackend()
	return nil
}

// watchBackend blocks until the Myna session ends and then closes the
// user-facing connection, unblocking the server read loop.
func (s *Session) watchBackend() {
	<-s.backend.Done()
	log.Printf("Backend closed, closing user connection")
	s.Close()
}

// Close tears down the backend session and the user connection. It is safe to
// call multiple times and from multiple goroutines.
func (s *Session) Close() error {
	s.closeOnce.Do(func() {
		if s.backend != nil {
			_ = s.backend.Close()
		}
		_ = s.Connection.Close()
	})
	return nil
}

// HandleMessage decodes a raw text frame and dispatches it to the handler that
// owns the required client state.
func (s *Session) HandleMessage(payload []byte) error {
	event, err := events.FromJson(payload)
	if err != nil {
		return s.SendError(
			events.ErrorTypeInvalidRequest,
			events.ErrorCodeInvalidParameter,
			fmt.Sprintf("decoding message: %s", err),
		)
	}

	switch eventType := event.(type) {
	case *events.SessionUpdate:
		return s.handleSessionUpdate(eventType)
	case *events.InputAudioBufferAppend:
		return s.handleInputAudioBufferAppend(eventType)
	case *events.InputAudioBufferCommit:
		return s.handleInputAudioBufferCommit(eventType)
	default:
		return s.SendError(
			events.ErrorTypeInvalidRequest,
			events.ErrorCodeInvalidParameter,
			fmt.Sprintf("unexpected event type: %T", eventType),
		)
	}
}

func getMergedConfiguration(original *backends.SessionConfig, update *events.SessionUpdate) backends.SessionConfig {
	cfg := *original // Shallow copy

	if update.Session.Type != nil {
		cfg.Type = *update.Session.Type
	}
	if update.Session.Instructions != nil {
		cfg.Instructions = update.Session.Instructions
	}
	if update.Session.Prompt != nil {
		cfg.Prompt = update.Session.Prompt
	}
	if update.Session.Include != nil {
		cfg.Include = update.Session.Include
	}
	if update.Session.Audio != nil && update.Session.Audio.Input != nil {
		if update.Session.Audio.Input.Format != nil {
			cfg.SampleRate = update.Session.Audio.Input.Format.Rate
		}
		if update.Session.Audio.Input.Transcription != nil {
			if update.Session.Audio.Input.Transcription.Model != nil {
				cfg.Model = *update.Session.Audio.Input.Transcription.Model
			}
			if update.Session.Audio.Input.Transcription.Language != nil {
				cfg.Lang = *update.Session.Audio.Input.Transcription.Language
			}
		}
	}

	return cfg
}

// handleSessionUpdate merges the requested session patch and acknowledges it
// with a session.updated event.
func (s *Session) handleSessionUpdate(m *events.SessionUpdate) error {
	// Validate request
	if err := m.Validate(); err != nil {
		return s.SendError(
			events.ErrorTypeInvalidRequest,
			events.ErrorCodeInvalidParameter,
			fmt.Sprintf("invalid session update: %s", err),
		)
	}

	// Merge configurations
	mergedConfig := getMergedConfiguration(s.backend.GetConfig(), m)

	// Validate merged config against the backend
	ok, err := s.backend.ValidateSessionConfig(mergedConfig)
	if err != nil {
		return s.SendError(
			events.ErrorTypeServer,
			events.ErrorCodeServerError,
			fmt.Sprintf("validating session configuration: %s", err),
		)
	} else if !ok {
		return s.SendError(
			events.ErrorTypeInvalidRequest,
			events.ErrorCodeInvalidParameter,
			fmt.Sprintf("invalid session configuration: %s", err),
		)
	}

	// Reject changes if currently transcribing
	if s.sessionStarted {
		return s.SendError(
			events.ErrorTypeInvalidRequest,
			events.ErrorCodeInvalidParameter,
			"session updates are not allowed after audio has been sent",
		)
	}

	// Apply the merged configuration to the backend
	err = s.backend.SetConfig(mergedConfig)

	if err != nil {
		return s.SendError(
			events.ErrorTypeServer,
			events.ErrorCodeServerError,
			fmt.Sprintf("setting backend config: %s", err),
		)
	}

	return s.send(events.NewSessionUpdated(s.backend.GetConfig()))
}

// Sends an error event on the user connection.
// The error is also returned to the caller for convenience.
func (s *Session) SendError(errorType string, errorCode string, message string) error {
	s.send(events.NewError(errorType, errorCode, message))
	return fmt.Errorf("%s", message)
}

// handleInputAudioBufferAppend decodes a base64 PCM16 chunk and forwards it to
// the WhisperLive backend.
func (s *Session) handleInputAudioBufferAppend(m *events.InputAudioBufferAppend) error {
	pcm, err := base64.StdEncoding.DecodeString(m.Audio)
	if err != nil {
		return s.SendError(
			events.ErrorTypeInvalidRequest,
			events.ErrorCodeInvalidParameter,
			"audio is not valid base64",
		)
	}
	if len(pcm) == 0 {
		return nil
	}

	// TODO: this does not follow the specs, which say that the server should accept audio
	// chunks even if other speech has already been committed.
	if s.audioBufferCommitted {
		return s.SendError(
			events.ErrorTypeServer,
			events.ErrorCodeServerError,
			"appending to a committed audio buffer is not supported",
		)
	}
	if s.backend == nil {
		return s.SendError(
			events.ErrorTypeInvalidRequest,
			events.ErrorCodeNoModelError,
			"backend session not started",
		)
	}
	if !s.modelLoaded {
		return s.SendError(
			events.ErrorTypeInvalidRequest,
			events.ErrorCodeNoModelError,
			"no model loaded",
		)
	}

	s.mu.Lock()
	s.sessionStarted = true
	s.mu.Unlock()

	if err := s.backend.SendPCM16(pcm); err != nil {
		return fmt.Errorf("forwarding audio to backend: %w", err)
	}
	return nil
}

// handleInputAudioBufferCommit signals end of audio input. It launches the
// finalization sequence in a goroutine: wait until the backend is idle (so the
// final segment is not dropped), send END_OF_AUDIO, then wait for the server to
// close — at which point watchBackend tears down the user connection.
func (s *Session) handleInputAudioBufferCommit(_ *events.InputAudioBufferCommit) error {
	if s.backend == nil {
		return s.SendError(
			events.ErrorTypeInvalidRequest,
			events.ErrorCodeNoModelError,
			"backend session not started",
		)
	}
	if s.audioBufferCommitted {
		return s.SendError(
			events.ErrorTypeServer,
			events.ErrorCodeServerError,
			"audio buffer is already committed",
		)
	}
	if !s.modelLoaded {
		return s.SendError(
			events.ErrorTypeInvalidRequest,
			events.ErrorCodeNoModelError,
			"no model loaded",
		)
	}
	s.mu.Lock()
	s.audioBufferCommitted = true
	s.mu.Unlock()
	go func() {
		if err := s.backend.Finalize(s.ctx); err != nil {
			log.Printf("[WARN]: finalizing backend: %v", err)
		}
	}()
	return nil
}

// onDelta is called by the backend whenever a new text fragment is available
// for the current partial segment.
func (s *Session) onDelta(delta string) {
	if err := s.send(events.NewTranscriptionDelta(delta)); err != nil {
		log.Printf("[WARN]: sending transcription delta: %v", err)
	}
}

// onCommit is called by the backend when a segment is finalized. An empty text
// signals a partial reset (the backend revised the in-progress text).
func (s *Session) onCommit(text string) {
	if err := s.send(events.NewTranscriptionCompleted(text)); err != nil {
		log.Printf("[WARN]: sending transcription completed: %v", err)
	}

	if s.audioBufferCommitted {
		log.Printf("Received onCommit after audio buffer finalized.\n")
		// At this point we could close the connection and consider the session done
		// but if we close the connection here, data is not flushed to the client and the
		// transcription completed event we just sent is lost along the way
		// so we let the client close the connection after receiving the transcription completed event
	}
}

// Invoked when the backend has loaded a model.
func (s *Session) onModelLoaded() {
	s.mu.Lock()
	s.modelLoaded = true
	s.mu.Unlock()
	// Send a new ModelLoaded event to the user
	err := s.send(new(events.ModelLoaded))
	if err != nil {
		log.Printf("[WARN]: sending model loaded: %v", err)
	}
}

// Invoked when the backend has unloaded a model.
func (s *Session) onModelUnloaded() {
	s.mu.Lock()
	s.modelLoaded = false
	s.mu.Unlock()
	// Send a new ModelUnloaded event to the user
	err := s.send(new(events.ModelUnloaded))
	if err != nil {
		log.Printf("[WARN]: sending model unloaded: %v", err)
	}
}

// send serializes an outbound event and writes it to the user websocket.
func (s *Session) send(m events.Event) error {
	data, err := events.ToJson(m)
	if err != nil {
		return fmt.Errorf("encoding event: %w", err)
	}
	s.writeMu.Lock()
	defer s.writeMu.Unlock()
	return s.Connection.WriteMessage(websocket.TextMessage, data)
}

// sessionFromCreated seeds local session state from the advertised defaults so
// later session.update patches merge against a known baseline.
func sessionFromCreated(m *events.SessionCreated) events.SessionData {
	return events.SessionData{
		Type:         m.Session.Type,
		Instructions: m.Session.Instructions,
		Prompt:       m.Session.Prompt,
		Audio: &events.SessionAudio{
			Input: &events.SessionAudioInput{
				Format: &events.SessionAudioFormat{Rate: m.Session.Audio.Input.Format.Rate},
				Transcription: &events.SessionTranscription{
					Model:    m.Session.Audio.Input.Transcription.Model,
					Language: m.Session.Audio.Input.Transcription.Language,
				},
			},
		},
		Include: m.Session.Include,
	}
}
