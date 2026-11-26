package main

import (
	"bufio"
	"context"
	"encoding/base64"
	"errors"
	"fmt"
	"io"
	"net"
	"os"
	"os/exec"
	"strings"
	"time"

	"myna-adapter/openai/events"

	"github.com/gorilla/websocket"
	"github.com/spf13/cobra"
)

type useAdapterCommand struct {
	url            string
	unixSocket     string
	audioFile      string
	audioDevice    string
	sampleRate     int
	chunkBytes     int
	timeoutSec     int
	realtimeFactor float64
	alreadyChanged bool
	model          string

	isFileStream bool
	ctx          *context.Context
}

// NewUseAdapterCmd builds a small test client. It connects to a running
// Myna Adapter websocket server, streams a local audio file as
// input_audio_buffer.append events paced in real time, then commits and prints
// the transcription events as they arrive.
func NewUseAdapterCmd() *cobra.Command {
	var cmd useAdapterCommand

	cobraCmd := &cobra.Command{
		Use:               "use-adapter",
		Short:             "Stream audio to a Myna Adapter server and print transcriptions",
		Long:              "Connect to a running Myna Adapter websocket server, stream microphone input or a local audio file (decoded with ffmpeg) as input_audio_buffer.append events, commit, and print transcription deltas/completions. Requires ffmpeg.",
		Args:              cobra.NoArgs,
		ValidArgsFunction: cobra.NoFileCompletions,
		RunE:              cmd.run,
	}

	cobraCmd.Flags().StringVar(&cmd.url, "url", "ws://127.0.0.1:8080/v1/realtime", "Myna Adapter websocket URL")
	cobraCmd.Flags().StringVar(&cmd.unixSocket, "unix-socket", "", "path to a Unix domain socket to connect to")
	cobraCmd.Flags().StringVar(&cmd.audioFile, "audio-file", "", "path to local audio file to stream. If no audio file is specified, microphone input will be used.")
	cobraCmd.Flags().StringVar(&cmd.audioDevice, "audio-device", "default", "PulseAudio device to use for input")
	cobraCmd.Flags().IntVar(&cmd.sampleRate, "rate", 16000, "sample rate to resample the audio to")
	cobraCmd.Flags().IntVar(&cmd.chunkBytes, "chunk-bytes", 4096, "PCM16 bytes per append event")
	cobraCmd.Flags().IntVar(&cmd.timeoutSec, "timeout-sec", 180, "overall command timeout in seconds")
	cobraCmd.Flags().StringVar(&cmd.model, "model", "base", "model name to switch to")
	cobraCmd.Flags().Float64Var(&cmd.realtimeFactor, "realtime-factor", 1.0, "factor to adjust real-time pacing of audio streaming")

	cobraCmd.MarkFlagsMutuallyExclusive("unix-socket", "url")
	cobraCmd.MarkFlagsMutuallyExclusive("audio-file", "audio-device")
	return cobraCmd
}

func (cmd *useAdapterCommand) run(cobraCmd *cobra.Command, _ []string) error {
	cmd.isFileStream = cmd.audioFile != ""
	if cmd.isFileStream {
		if _, err := os.Stat(cmd.audioFile); err != nil {
			return fmt.Errorf("audio file not accessible: %w", err)
		}
	}
	if strings.TrimSpace(cmd.url) == "" && strings.TrimSpace(cmd.unixSocket) == "" {
		return errors.New("either --url or --unix-socket is required")
	}
	if cmd.realtimeFactor < 1 {
		return errors.New("realtime-factor must be >= 1")
	}

	ctx, cancel := context.WithTimeout(context.Background(), time.Duration(cmd.timeoutSec)*time.Second)
	defer cancel()
	cmd.ctx = &ctx

	out := cobraCmd.OutOrStdout()

	dialer := *websocket.DefaultDialer
	unixSocket := strings.TrimSpace(cmd.unixSocket)
	unixSocket = strings.TrimPrefix(unixSocket, "unix://") // remove unix:// prefix if present

	if unixSocket != "" {
		fmt.Fprintf(out, "connecting to unix socket %s\n", unixSocket)
		dialer.NetDialContext = func(ctx context.Context, _, _ string) (net.Conn, error) {
			return (&net.Dialer{}).DialContext(ctx, "unix", unixSocket)
		}
		cmd.url = "ws://unix/v1/realtime" // host is ignored when using unix socket, but must be a valid URL
	} else {
		fmt.Fprintf(out, "connecting to %s\n", cmd.url)
	}

	conn, _, err := dialer.DialContext(ctx, cmd.url, nil)
	if err != nil {
		return fmt.Errorf("dial myna server: %w", err)
	}
	defer conn.Close()

	// Read events in the background and print them until the connection closes.
	done := make(chan struct{})
	go func() {
		defer close(done)
		cmd.readLoop(out, conn)
	}()

	// Wait for the server's session.created before streaming audio.
	fmt.Fprintln(out, "waiting for session.created")

	// The server flushes the final segment and closes the connection on commit,
	// which ends the read loop. Bound the wait by the context.
	select {
	case <-done:
	case <-ctx.Done():
		return ctx.Err()
	}
	return nil
}

func (cmd *useAdapterCommand) sendAudio(out io.Writer, conn *websocket.Conn) error {
	// Stream the audio, then commit to signal end of input.
	if err := cmd.stream(out, conn); err != nil {
		return fmt.Errorf("streaming audio: %w", err)
	}

	commit, err := events.ToJson(&events.InputAudioBufferCommit{})
	if err != nil {
		return fmt.Errorf("encoding commit: %w", err)
	}
	if err := conn.WriteMessage(websocket.TextMessage, commit); err != nil {
		return fmt.Errorf("sending commit: %w", err)
	}

	fmt.Fprintf(out, "Audio streaming completed\n")
	return nil
}

// stream decodes the audio file to PCM16 mono with ffmpeg and sends it as
// base64 input_audio_buffer.append events, paced at real time.
func (cmd *useAdapterCommand) stream(out io.Writer, conn *websocket.Conn) error {
	ffmpeg_parameters := []string{
		"-hide_banner", "-loglevel", "error",
		"-ac", "1",
		"-ar", fmt.Sprintf("%d", cmd.sampleRate),
		"-f", "s16le",
		"pipe:1",
	}

	if cmd.isFileStream {
		ffmpeg_parameters = append([]string{"-i", cmd.audioFile}, ffmpeg_parameters...)
	} else {
		ffmpeg_parameters = append([]string{"-f", "pulse", "-i", cmd.audioDevice}, ffmpeg_parameters...)
	}

	ff := exec.CommandContext(*cmd.ctx, "ffmpeg", ffmpeg_parameters...)
	stdout, err := ff.StdoutPipe()
	if err != nil {
		return fmt.Errorf("ffmpeg stdout pipe: %w", err)
	}
	stderr := &strings.Builder{}
	ff.Stderr = stderr

	if err := ff.Start(); err != nil {
		return fmt.Errorf("start ffmpeg: %w", err)
	}

	if cmd.isFileStream {
		fmt.Fprintf(out, "streaming %s\n", cmd.audioFile)
	} else {
		fmt.Fprintf(out, "streaming from device %s\n", cmd.audioDevice)
	}

	reader := bufio.NewReader(stdout)
	chunk := make([]byte, cmd.chunkBytes)

	for {
		n, readErr := io.ReadFull(reader, chunk)
		if n > 0 {
			payload := chunk[:n]
			msg, err := events.ToJson(&events.InputAudioBufferAppend{
				Audio: base64.StdEncoding.EncodeToString(payload),
			})
			if err != nil {
				return fmt.Errorf("encoding append: %w", err)
			}
			if err := conn.WriteMessage(websocket.TextMessage, msg); err != nil {
				return fmt.Errorf("sending append: %w", err)
			}
			if !cmd.isFileStream {
				// Pace at real time: s16le mono => 2 bytes per sample.
				samples := n / 2
				pace := time.Duration(float64(samples) / float64(cmd.sampleRate) * float64(time.Second) / cmd.realtimeFactor)
				select {
				case <-(*cmd.ctx).Done():
					return (*cmd.ctx).Err()
				case <-time.After(pace):
				}
			}
		}
		if readErr != nil {
			if readErr == io.EOF || readErr == io.ErrUnexpectedEOF {
				break
			}
			return readErr
		}
	}

	if err := ff.Wait(); err != nil {
		if msg := strings.TrimSpace(stderr.String()); msg != "" {
			return fmt.Errorf("ffmpeg failed: %w (%s)", err, msg)
		}
		return fmt.Errorf("ffmpeg failed: %w", err)
	}
	return nil
}

// readLoop prints inbound server events until the connection closes.
func (cmd *useAdapterCommand) readLoop(out io.Writer, conn *websocket.Conn) {
	for {
		_, payload, err := conn.ReadMessage()
		if err != nil {
			return
		}

		msg, err := events.FromJson(payload)
		if err != nil {
			fmt.Fprintf(out, "Error unmarshaling received payload: %v\n", string(payload))
			continue
		}

		switch m := msg.(type) {
		case *events.SessionCreated:
			fmt.Fprintf(out, "-> received [session.created]: %v\n", string(payload))
		case *events.SessionUpdated:
			fmt.Fprintf(out, "-> received [session.updated]: %v\n", string(payload))
		case *events.ConversationItemInputAudioTranscriptionDelta:
			fmt.Fprintf(out, "-> received [delta]: %q\n", m.Delta)
		case *events.ModelLoaded:
			fmt.Fprintf(out, "-> received [model.loaded]\n")
			// send a session update
			if !cmd.alreadyChanged {
				cmd.alreadyChanged = true
				fmt.Fprintf(out, "<- sending  [session.update]...\n")
				go func() {
					update, err := events.ToJson(&events.SessionUpdate{
						Session: events.SessionData{
							Audio: &events.SessionAudio{
								Input: &events.SessionAudioInput{
									Transcription: &events.SessionTranscription{
										Model: new(cmd.model),
									},
								},
							},
						},
					})
					if err != nil {
						fmt.Fprintf(out, "error encoding session update: %v\n", err)
						return
					}
					if err := conn.WriteMessage(websocket.TextMessage, update); err != nil {
						fmt.Fprintf(out, "Error sending [session.update]: %v\n", err)
						return
					}

					fmt.Fprintf(out, "<- sent     [session.update]\n")
				}()
			} else {
				go func() {
					if err := cmd.sendAudio(out, conn); err != nil {
						fmt.Fprintf(out, "Error sending audio: %v\n", err)
					}
					fmt.Fprintln(out, "Audio committed, waiting for final transcription")
				}()
			}
		case *events.ModelUnloaded:
			fmt.Fprintf(out, "-> received [model.unloaded]\n")
		case *events.ConversationItemInputAudioTranscriptionCompleted:
			fmt.Fprintf(out, "-> received [completed]: %q\n", m.Transcript)
		case *events.Error:
			fmt.Fprintf(out, "-> received [error]: %s/%s: %s\n", m.Error.Type, m.Error.Code, m.Error.Message)
		default:
			fmt.Fprintf(out, "[%T]\n", m)
		}
	}
}
