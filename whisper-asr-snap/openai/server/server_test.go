package server

import (
	"errors"
	"net"
	"sync"
	"testing"
	"time"
)

// fakeAddr is a minimal net.Addr for use by fakeListener.
type fakeAddr string

func (a fakeAddr) Network() string { return "fake" }
func (a fakeAddr) String() string  { return string(a) }

// fakeListener lets tests control exactly when/how Accept fails, without
// binding real sockets.
type fakeListener struct {
	addr      fakeAddr
	acceptErr error // if set, Accept returns this immediately (simulates a runtime failure)

	closeOnce sync.Once
	closed    chan struct{}
}

func newFakeListener(addr string, acceptErr error) *fakeListener {
	return &fakeListener{addr: fakeAddr(addr), acceptErr: acceptErr, closed: make(chan struct{})}
}

func (f *fakeListener) Accept() (net.Conn, error) {
	if f.acceptErr != nil {
		return nil, f.acceptErr
	}
	<-f.closed
	return nil, errors.New("listener closed")
}

func (f *fakeListener) Close() error {
	f.closeOnce.Do(func() { close(f.closed) })
	return nil
}

func (f *fakeListener) Addr() net.Addr { return f.addr }

// TestStart_OneListenerFailing_ClosesTheOthers verifies that when one of
// several listeners fails at runtime (not at bind time), Start tears down
// every other listener and returns promptly instead of hanging with the
// server left running in a degraded state.
func TestStart_OneListenerFailing_ClosesTheOthers(t *testing.T) {
	good := newFakeListener("good:1", nil)
	bad := newFakeListener("bad:1", errors.New("boom"))

	s := &WebSocketServer{
		bindings: []binding{
			{network: "tcp", address: "good:1"},
			{network: "tcp", address: "bad:1"},
		},
		listen: func(_, address string) (net.Listener, error) {
			switch address {
			case "good:1":
				return good, nil
			case "bad:1":
				return bad, nil
			}
			return nil, errors.New("unexpected address")
		},
	}

	done := make(chan error, 1)
	go func() { done <- s.Start() }()

	select {
	case err := <-done:
		if err == nil || err.Error() != "boom" {
			t.Fatalf("expected Start to return the failing listener's error, got %v", err)
		}
	case <-time.After(2 * time.Second):
		t.Fatal("Start did not return after one listener failed; the healthy listener was not closed")
	}

	select {
	case <-good.closed:
	default:
		t.Fatal("the healthy listener was not closed when its sibling failed")
	}
}
