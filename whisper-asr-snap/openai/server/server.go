package server

import (
	"context"
	"errors"
	"fmt"
	"net"
	"net/http"
	"os"
	"strconv"
	"sync"
	"time"

	"myna-adapter/backends"
	"myna-adapter/openai/server/endpoints"

	"github.com/gorilla/websocket"
)

// binding describes a single network/address pair the server listens on.
type binding struct {
	network string
	address string
}

func (b binding) displayAddress() string {
	if b.network == "unix" {
		return "unix://" + b.address
	}
	return b.address
}

type WebSocketServer struct {
	bindings []binding

	factory       backends.Factory
	allowedModels []string
	startTime     time.Time

	upgrader websocket.Upgrader
	httpSrv  *http.Server
	mu       sync.Mutex
	running  bool

	// listen is overridable in tests to simulate listener failures.
	listen func(network, address string) (net.Listener, error)
}

// NewWebSocketServer creates a server that listens on TCP (host/port), if
// port is > 0, and on a Unix domain socket, if unixSocketPath is
// non-empty. Both may be enabled at the same time, but at least one must be.
func NewWebSocketServer(host string, port int, unixSocketPath string) *WebSocketServer {
	var bindings []binding

	if port > 0 {
		bindings = append(bindings, binding{network: "tcp", address: net.JoinHostPort(host, strconv.Itoa(port))})
	}

	if unixSocketPath != "" {
		bindings = append(bindings, binding{network: "unix", address: unixSocketPath})
	}

	return &WebSocketServer{
		bindings:  bindings,
		startTime: time.Now(),
		upgrader: websocket.Upgrader{
			CheckOrigin: func(r *http.Request) bool { return true },
		},
		listen: net.Listen,
	}
}

// SetBackend configures the session metadata and the factory used to open a
// backend session for each connecting user.
func (s *WebSocketServer) SetBackend(cfg backends.SessionConfig, factory backends.Factory) {
	s.factory = factory
}

// SetAllowedModels configures the list of model names advertised by the
// /v1/models endpoint.
func (s *WebSocketServer) SetAllowedModels(models []string) {
	s.allowedModels = models
}

// Addresses returns the display addresses of every listener the server binds to.
func (s *WebSocketServer) Addresses() []string {
	addrs := make([]string, len(s.bindings))
	for i, b := range s.bindings {
		addrs[i] = b.displayAddress()
	}
	return addrs
}

func (s *WebSocketServer) Start() error {
	s.mu.Lock()
	if s.running {
		s.mu.Unlock()
		return fmt.Errorf("server already running")
	}

	mux := http.NewServeMux()
	mux.HandleFunc("/v1/realtime", endpoints.Realtime(s.upgrader, func(conn *websocket.Conn) endpoints.RealtimeSession {
		return NewSession(conn, s.factory)
	}))
	mux.HandleFunc("/v1/models", endpoints.Models(s.allowedModels, s.startTime))
	mux.HandleFunc("/{$}", endpoints.Root())

	s.httpSrv = &http.Server{Handler: mux}
	s.running = true
	s.mu.Unlock()

	listeners := make([]net.Listener, 0, len(s.bindings))
	unixSocketPaths := make([]string, 0, len(s.bindings))
	cleanup := func() {
		for _, l := range listeners {
			l.Close()
		}
		for _, path := range unixSocketPaths {
			os.Remove(path)
		}
		s.mu.Lock()
		s.running = false
		s.httpSrv = nil
		s.mu.Unlock()
	}

	for _, b := range s.bindings {
		if b.network == "unix" {
			if err := os.Remove(b.address); err != nil && !errors.Is(err, os.ErrNotExist) {
				cleanup()
				return fmt.Errorf("removing existing unix socket %q: %w", b.address, err)
			}
		}

		listener, err := s.listen(b.network, b.address)
		if err != nil {
			cleanup()
			return err
		}

		if b.network == "unix" {
			unixSocketPaths = append(unixSocketPaths, b.address)
		}

		fmt.Printf("http server now listening on %s\n", b.displayAddress())

		listeners = append(listeners, listener)
	}

	errCh := make(chan error, len(listeners))
	for _, listener := range listeners {
		go func(l net.Listener) {
			err := s.httpSrv.Serve(l)
			if err == http.ErrServerClosed {
				err = nil
			}
			errCh <- err
		}(listener)
	}

	var firstErr error
	for range listeners {
		if err := <-errCh; err != nil && firstErr == nil {
			firstErr = err
			// a listener failed on its own (not via Stop); tear down the
			// others so the server doesn't keep running in a degraded state.
			s.httpSrv.Close()
		}
	}

	for _, b := range s.bindings {
		if b.network == "unix" {
			os.Remove(b.address)
		}
	}

	s.mu.Lock()
	s.running = false
	s.httpSrv = nil
	s.mu.Unlock()

	return firstErr
}

func (s *WebSocketServer) Stop(ctx context.Context) error {
	s.mu.Lock()
	srv := s.httpSrv
	running := s.running
	s.mu.Unlock()

	if srv == nil || !running {
		return nil
	}

	return srv.Shutdown(ctx)
}
