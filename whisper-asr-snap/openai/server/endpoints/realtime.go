// Package endpoints contains the HTTP handlers exposed by the
// OpenAI-compatible server, decoupled from connection/lifecycle management
// so each endpoint can be tested in isolation.
package endpoints

import (
	"context"
	"fmt"
	"net/http"
	"os"

	"myna-adapter/openai/events"

	"github.com/gorilla/websocket"
)

// RealtimeSession is the subset of session behaviour the realtime endpoint
// depends on. *server.Session implements this interface.
type RealtimeSession interface {
	// Start opens the backend session and advertises session.created.
	Start(ctx context.Context) error
	// Close tears down the session.
	Close() error
	// HandleMessage processes a single text frame received from the client.
	HandleMessage(payload []byte) error
	// SendError sends an OpenAI-formatted error event to the client.
	SendError(errorType string, errorCode string, message string) error
}

// Realtime returns the /v1/realtime WebSocket handler. newSession is invoked
// once per accepted connection to construct the session driving it.
func Realtime(upgrader websocket.Upgrader, newSession func(conn *websocket.Conn) RealtimeSession) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		conn, err := upgrader.Upgrade(w, r, nil)
		if err != nil {
			http.Error(w, err.Error(), http.StatusBadRequest)
			return
		}
		defer conn.Close()

		session := newSession(conn)
		defer session.Close()

		// Open the backend session and advertise session.created before accepting
		// audio from the user.
		if err := session.Start(r.Context()); err != nil {
			fmt.Printf("starting client session: %v\n", err)
			_ = session.SendError(
				events.ErrorTypeServer,
				events.ErrorCodeServerError,
				"failed to start session",
			)
			return
		}

		for {
			messageType, payload, err := conn.ReadMessage()
			if err != nil {
				fmt.Printf("reading message: %v\n", err)
				return
			}

			switch messageType {
			case websocket.BinaryMessage:
				// Send error for unsupported binary frames
				_ = session.SendError(
					events.ErrorTypeInvalidRequest,
					events.ErrorCodeInvalidParameter,
					"binary messages are unsupported",
				)

			case websocket.TextMessage:
				if err := session.HandleMessage(payload); err != nil {
					fmt.Fprintf(os.Stderr, "Error handling message: %v\n", err)
				}
			}
		}
	}
}
