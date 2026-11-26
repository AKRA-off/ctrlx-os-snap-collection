package endpoints

import "net/http"

// Root returns the handler for "/" liveness check.
func Root() http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
		w.Write([]byte("ok\n"))
	}
}
