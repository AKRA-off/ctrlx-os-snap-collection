// Package backends defines the interface that all transcription backends must
// implement, plus the factory type used to open per-connection sessions.
package backends

import "context"

type Backend interface {
	// WaitReady blocks until the backend is ready to receive audio.
	WaitReady(ctx context.Context) error
	// Done returns a channel closed when the backend session has ended.
	Done() <-chan struct{}
	// Close tears down the backend session.
	Close() error
	// SendPCM16 forwards a chunk of little-endian signed 16-bit mono PCM audio.
	SendPCM16(pcm []byte) error
	// Finalize signals the end of audio input and waits for the backend to flush.
	Finalize(ctx context.Context) error
	// ValidateSessionConfig checks if the given session config is valid for this backend.
	ValidateSessionConfig(c SessionConfig) (bool, error)
	// GetConfig returns the current session configuration of the backend.
	GetConfig() *SessionConfig
	// SetConfig applies a new session configuration to the backend. Depending on the backend implementation, this may reload the backend session.
	SetConfig(c SessionConfig) error
}

type BackendCallbacks struct {
	// OnDelta is called when the backend has a new partial transcription.
	OnDelta func(string)
	// OnCommit is called when the backend has a finalized transcription.
	OnCommit func(string)
	// OnModelLoaded is called when the backend has loaded a model.
	OnModelLoaded func()
	// OnModelUnloaded is called when the backend has unloaded a model.
	OnModelUnloaded func()
}

// Factory is a function that opens a new Backend session
type Factory func(ctx context.Context, callbacks BackendCallbacks) (Backend, error)

type SessionConfig struct {
	Type         string
	Instructions *string
	Prompt       *string
	Include      []string
	Model        string
	Lang         string
	AudioFormat  string
	SampleRate   int
}
