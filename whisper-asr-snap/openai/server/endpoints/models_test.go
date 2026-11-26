package endpoints

import (
	"encoding/json"
	"net/http/httptest"
	"testing"
	"time"
)

// TestModels_ListsAllowedModels verifies that the /v1/models handler returns
// an OpenAI-compatible response listing exactly the configured allowed
// models.
func TestModels_ListsAllowedModels(t *testing.T) {
	handler := Models([]string{"small", "tiny"}, time.Now())

	req := httptest.NewRequest("GET", "/v1/models", nil)
	rec := httptest.NewRecorder()

	handler(rec, req)

	if rec.Code != 200 {
		t.Fatalf("expected status 200, got %d", rec.Code)
	}

	var got ModelList
	if err := json.Unmarshal(rec.Body.Bytes(), &got); err != nil {
		t.Fatalf("unmarshalling response: %v", err)
	}

	if got.Object != "list" {
		t.Fatalf("expected object %q, got %q", "list", got.Object)
	}

	if len(got.Data) != 2 {
		t.Fatalf("expected 2 models, got %d: %+v", len(got.Data), got.Data)
	}

	ids := []string{got.Data[0].ID, got.Data[1].ID}
	if ids[0] != "small" || ids[1] != "tiny" {
		t.Fatalf("expected models [small tiny] in order, got %v", ids)
	}

	for _, m := range got.Data {
		if m.Object != "model" {
			t.Errorf("expected object %q for model %q, got %q", "model", m.ID, m.Object)
		}
		if m.OwnedBy == "" {
			t.Errorf("expected non-empty owned_by for model %q", m.ID)
		}
	}
}

// TestModels_MethodNotAllowed verifies non-GET requests are rejected.
func TestModels_MethodNotAllowed(t *testing.T) {
	handler := Models([]string{"small"}, time.Now())

	req := httptest.NewRequest("POST", "/v1/models", nil)
	rec := httptest.NewRecorder()

	handler(rec, req)

	if rec.Code != 405 {
		t.Fatalf("expected status 405, got %d", rec.Code)
	}
}
