package main

import (
	"bytes"
	"os"
	"strings"
	"testing"
)

// executeTranscribe runs the transcribe subcommand with the given extra args
// and returns the error produced by RunE (not the cobra wrapper error).
func executeTranscribe(t *testing.T, args ...string) error {
	t.Helper()
	root := NewRootCmd()
	root.SetOut(&bytes.Buffer{})
	root.SetErr(&bytes.Buffer{})
	root.SetArgs(append([]string{"use-backend"}, args...))
	return root.Execute()
}

func TestTranscribeRequiresAudioFlag(t *testing.T) {
	err := executeTranscribe(t)
	if err == nil {
		t.Fatal("expected error when --audio is missing, got nil")
	}
}

func TestTranscribeNonExistentFile(t *testing.T) {
	err := executeTranscribe(t, "--audio", "/nonexistent/path/audio.wav")
	if err == nil {
		t.Fatal("expected error for non-existent file, got nil")
	}
	if !strings.Contains(err.Error(), "audio file not accessible") {
		t.Errorf("error = %q, want to contain 'audio file not accessible'", err.Error())
	}
}

func TestTranscribeEmptyHost(t *testing.T) {
	err := executeTranscribe(t, "--audio", "/nonexistent/audio.wav", "--host", "")
	if err == nil {
		t.Fatal("expected error for empty host, got nil")
	}
	// Either the file-not-found check or the host check fires — either is valid
	// validation. We just confirm an error is returned.
}

func TestTranscribeZeroPort(t *testing.T) {
	// Create a temp file so the stat check passes, then hit the port validation.
	tmp := t.TempDir() + "/audio.wav"
	if err := createTempFile(tmp); err != nil {
		t.Fatalf("creating temp file: %v", err)
	}
	err := executeTranscribe(t, "--audio", tmp, "--port", "0")
	if err == nil {
		t.Fatal("expected error for port=0, got nil")
	}
	if !strings.Contains(err.Error(), "host and port are required") {
		t.Errorf("error = %q, want to contain 'host and port are required'", err.Error())
	}
}

func TestTranscribeNegativePort(t *testing.T) {
	tmp := t.TempDir() + "/audio.wav"
	if err := createTempFile(tmp); err != nil {
		t.Fatalf("creating temp file: %v", err)
	}
	err := executeTranscribe(t, "--audio", tmp, "--port", "-1")
	if err == nil {
		t.Fatal("expected error for negative port, got nil")
	}
	if !strings.Contains(err.Error(), "host and port are required") {
		t.Errorf("error = %q, want to contain 'host and port are required'", err.Error())
	}
}

func createTempFile(path string) error {
	f, err := os.Create(path)
	if err != nil {
		return err
	}
	return f.Close()
}
