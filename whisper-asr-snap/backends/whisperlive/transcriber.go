package whisperlive

import (
	"bufio"
	"context"
	"encoding/binary"
	"fmt"
	"io"
	"os/exec"
	"strings"
	"time"
)

// TranscribeFile decodes a local audio file to 16 kHz mono PCM with ffmpeg and
// streams it to the backend. It blocks until the transcript has been finalized.
func (c *Client) TranscribeFile(ctx context.Context, path string) error {
	if strings.TrimSpace(path) == "" {
		return fmt.Errorf("audio path is required")
	}

	cmd := exec.CommandContext(ctx, "ffmpeg",
		"-hide_banner", "-loglevel", "error",
		"-i", path,
		"-ac", "1",
		"-ar", fmt.Sprintf("%d", c.cfg.SampleRate),
		"-f", "s16le",
		"pipe:1",
	)

	stdout, err := cmd.StdoutPipe()
	if err != nil {
		return fmt.Errorf("ffmpeg stdout pipe: %w", err)
	}
	stderr := &strings.Builder{}
	cmd.Stderr = stderr

	if err := cmd.Start(); err != nil {
		return fmt.Errorf("start ffmpeg: %w", err)
	}

	streamErr := c.stream(ctx, stdout)
	waitErr := cmd.Wait()

	if streamErr != nil {
		if msg := strings.TrimSpace(stderr.String()); msg != "" {
			return fmt.Errorf("%w (ffmpeg: %s)", streamErr, msg)
		}
		return streamErr
	}
	if waitErr != nil {
		if msg := strings.TrimSpace(stderr.String()); msg != "" {
			return fmt.Errorf("ffmpeg failed: %w (%s)", waitErr, msg)
		}
		return fmt.Errorf("ffmpeg failed: %w", waitErr)
	}
	return nil
}

// stream reads s16le PCM from r, sends it to the backend at real time, then runs
// the shutdown handshake so the backend flushes its final segment.
//
// The pacing (sleeping for each chunk's real duration) matters: dumping audio as
// fast as ffmpeg decodes it floods the backend, which then re-runs inference over
// the same buffered window and makes end-of-stream nondeterministic.
func (c *Client) stream(ctx context.Context, r io.Reader) error {
	reader := bufio.NewReader(r)
	chunk := make([]byte, c.cfg.ChunkBytes)

	for {
		n, readErr := io.ReadFull(reader, chunk)
		if n > 0 {
			// io.ReadFull leaves the final short read in chunk[:n]; send it so
			// the tail of the audio is not dropped.
			samples := pcm16ToFloat32(chunk[:n])
			if err := c.sendAudio(samples); err != nil {
				return err
			}
			pace := time.Duration(float64(len(samples)) / float64(c.cfg.SampleRate) * float64(time.Second))
			select {
			case <-ctx.Done():
				return ctx.Err()
			case <-c.closed:
				return c.streamClosedErr()
			case <-time.After(pace):
			}
		}
		if readErr != nil {
			if readErr == io.EOF || readErr == io.ErrUnexpectedEOF {
				break
			}
			return readErr
		}
	}

	// Shutdown handshake, in this exact order:
	//  1. wait until the backend has been idle (no new transcript) so buffered
	//     audio is transcribed before we signal the end;
	//  2. send END_OF_AUDIO so backends that only flush on it emit the last
	//     segment;
	//  3. give the backend a short grace window to deliver that segment / close.
	c.waitUntilIdle(ctx)
	_ = c.sendEndOfAudio()
	c.waitForClose(ctx)
	return nil
}

// waitUntilIdle blocks until no new transcript has arrived for IdleTimeout, the
// backend ends the session, or the context/connection is done.
func (c *Client) waitUntilIdle(ctx context.Context) {
	ticker := time.NewTicker(50 * time.Millisecond)
	defer ticker.Stop()
	for {
		select {
		case <-ctx.Done():
			return
		case <-c.closed:
			return
		case <-ticker.C:
			if !c.recordingActive() || c.idleFor() >= c.cfg.IdleTimeout {
				return
			}
		}
	}
}

// waitForClose blocks until the backend closes the connection or the grace
// period elapses.
func (c *Client) waitForClose(ctx context.Context) {
	timer := time.NewTimer(c.cfg.ServerCloseGrace)
	defer timer.Stop()
	select {
	case <-c.closed:
	case <-timer.C:
	case <-ctx.Done():
	}
}

func (c *Client) streamClosedErr() error {
	if msg := c.errorMessage(); msg != "" {
		return fmt.Errorf("server error: %s", msg)
	}
	return nil
}

func pcm16ToFloat32(b []byte) []float32 {
	samples := make([]float32, 0, len(b)/2)
	for i := 0; i+1 < len(b); i += 2 {
		s := int16(binary.LittleEndian.Uint16(b[i:]))
		samples = append(samples, float32(s)/32768.0)
	}
	return samples
}
