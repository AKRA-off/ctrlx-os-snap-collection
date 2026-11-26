package whisperlive

import (
	"myna-adapter/backends"
	"sync"
	"testing"
	"time"
)

// ── FinalTranscript ───────────────────────────────────────────────────────────

func newTestClient(cfg Config) *Client {
	return &Client{
		cfg:          cfg.withDefaults(),
		ready:        make(chan struct{}),
		closed:       make(chan struct{}),
		lastActivity: time.Now(),
	}
}

func TestFinalTranscriptEmpty(t *testing.T) {
	c := newTestClient(Config{})
	if got := c.FinalTranscript(); got != "" {
		t.Errorf("got %q, want %q", got, "")
	}
}

func TestFinalTranscriptCompletedSegmentsOnly(t *testing.T) {
	c := newTestClient(Config{})
	c.transcript = []Segment{
		{Text: " hello ", Completed: true},
		{Text: " world ", Completed: true},
	}
	got := c.FinalTranscript()
	if got != "hello world" {
		t.Errorf("got %q, want %q", got, "hello world")
	}
}

func TestFinalTranscriptPartialOnly(t *testing.T) {
	c := newTestClient(Config{})
	seg := Segment{Text: " partial "}
	c.partial = &seg
	got := c.FinalTranscript()
	if got != "partial" {
		t.Errorf("got %q, want %q", got, "partial")
	}
}

func TestFinalTranscriptCompletedPlusPartial(t *testing.T) {
	c := newTestClient(Config{})
	c.transcript = []Segment{{Text: " hello ", Completed: true}}
	seg := Segment{Text: " world "}
	c.partial = &seg
	got := c.FinalTranscript()
	if got != "hello world" {
		t.Errorf("got %q, want %q", got, "hello world")
	}
}

func TestFinalTranscriptPartialNotDuplicatedWhenMatchesLast(t *testing.T) {
	c := newTestClient(Config{})
	c.transcript = []Segment{{Text: "same", Completed: true}}
	seg := Segment{Text: "same"}
	c.partial = &seg
	got := c.FinalTranscript()
	if got != "same" {
		t.Errorf("got %q, want %q (partial should not duplicate last completed)", got, "same")
	}
}

// ── updateSegments ────────────────────────────────────────────────────────────

func TestUpdateSegmentsEmptyIsNoop(t *testing.T) {
	c := newTestClient(Config{})
	c.updateSegments(nil)
	if got := c.FinalTranscript(); got != "" {
		t.Errorf("got %q, want empty", got)
	}
}

func TestUpdateSegmentsSinglePartial(t *testing.T) {
	c := newTestClient(Config{})
	c.updateSegments([]Segment{
		{Start: 0, End: 1, Text: " hello ", Completed: false},
	})
	if got := c.FinalTranscript(); got != "hello" {
		t.Errorf("got %q, want %q", got, "hello")
	}
}

func TestUpdateSegmentsSingleCompleted(t *testing.T) {
	c := newTestClient(Config{})
	c.updateSegments([]Segment{
		{Start: 0, End: 1, Text: " done ", Completed: true},
	})
	got := c.FinalTranscript()
	if got != "done" {
		t.Errorf("got %q, want %q", got, "done")
	}
	// partial should be cleared
	c.mu.Lock()
	partial := c.partial
	c.mu.Unlock()
	if partial != nil {
		t.Errorf("partial = %v, want nil after completed segment", partial)
	}
}

func TestUpdateSegmentsDeduplicatesRepeatedCompletedSegments(t *testing.T) {
	c := newTestClient(Config{})
	seg := Segment{Start: 0, End: 1.0, Text: "hello", Completed: true}
	c.updateSegments([]Segment{seg})
	c.updateSegments([]Segment{seg}) // same segment re-sent by backend
	c.mu.Lock()
	n := len(c.transcript)
	c.mu.Unlock()
	if n != 1 {
		t.Errorf("transcript has %d segments, want 1 (dedup failed)", n)
	}
}

func TestUpdateSegmentsAppendsNonOverlappingCompleted(t *testing.T) {
	c := newTestClient(Config{})
	c.updateSegments([]Segment{
		{Start: 0, End: 1.0, Text: "first", Completed: true},
	})
	c.updateSegments([]Segment{
		{Start: 1.0, End: 2.0, Text: "second", Completed: true},
	})
	got := c.FinalTranscript()
	if got != "first second" {
		t.Errorf("got %q, want %q", got, "first second")
	}
}

func TestUpdateSegmentsOnDeltaCalledWithSuffix(t *testing.T) {
	var mu sync.Mutex
	var deltas []string
	c := newTestClient(Config{
		Callbacks: backends.BackendCallbacks{
			OnDelta: func(d string) {
				mu.Lock()
				defer mu.Unlock()
				deltas = append(deltas, d)
			},
		},
	})

	c.updateSegments([]Segment{{Start: 0, End: 1, Text: "hello", Completed: false}})
	c.updateSegments([]Segment{{Start: 0, End: 1, Text: "hello world", Completed: false}})

	mu.Lock()
	defer mu.Unlock()
	if len(deltas) != 2 {
		t.Fatalf("got %d delta calls, want 2: %v", len(deltas), deltas)
	}
	if deltas[0] != "hello" {
		t.Errorf("delta[0] = %q, want %q", deltas[0], "hello")
	}
	if deltas[1] != " world" {
		t.Errorf("delta[1] = %q, want %q", deltas[1], " world")
	}
}

func TestUpdateSegmentsOnDeltaRevisionResetsAndResends(t *testing.T) {
	var mu sync.Mutex
	var deltas []string
	var commits []string
	c := newTestClient(Config{
		Callbacks: backends.BackendCallbacks{
			OnDelta: func(d string) {
				mu.Lock()
				defer mu.Unlock()
				deltas = append(deltas, d)
			},
			OnCommit: func(text string) {
				mu.Lock()
				defer mu.Unlock()
				commits = append(commits, text)
			},
		},
	})

	c.updateSegments([]Segment{{Start: 0, End: 1, Text: "hello", Completed: false}})
	// Backend rewrote the partial; "world" doesn't extend "hello"
	c.updateSegments([]Segment{{Start: 0, End: 1, Text: "world", Completed: false}})

	mu.Lock()
	defer mu.Unlock()
	if len(commits) < 1 {
		t.Fatalf("expected at least one commit (reset), got %v", commits)
	}
	// First commit on revision is the empty-string reset
	if commits[0] != "" {
		t.Errorf("commit[0] = %q, want empty string (reset signal)", commits[0])
	}
	// The revised text should be re-sent as a full delta
	found := false
	for _, d := range deltas {
		if d == "world" {
			found = true
		}
	}
	if !found {
		t.Errorf("expected delta %q after revision, got deltas %v", "world", deltas)
	}
}

func TestUpdateSegmentsOnCommitCalledWhenSegmentCompletes(t *testing.T) {
	var mu sync.Mutex
	var commits []string
	c := newTestClient(Config{
		Callbacks: backends.BackendCallbacks{
			OnCommit: func(text string) {
				mu.Lock()
				defer mu.Unlock()
				commits = append(commits, text)
			},
		},
	})

	// partial first
	c.updateSegments([]Segment{{Start: 0, End: 1, Text: "hello", Completed: false}})
	// now a second segment appears, committing the first
	c.updateSegments([]Segment{
		{Start: 0, End: 1, Text: "hello", Completed: true},
		{Start: 1, End: 2, Text: "world", Completed: false},
	})

	mu.Lock()
	defer mu.Unlock()
	found := false
	for _, c := range commits {
		if c == "hello" {
			found = true
		}
	}
	if !found {
		t.Errorf("expected commit %q, got %v", "hello", commits)
	}
}

func TestUpdateSegmentsOnTranscriptionCalledWithFullText(t *testing.T) {
	var mu sync.Mutex
	var texts []string
	c := newTestClient(Config{
		OnTranscription: func(text string, _ []Segment) {
			mu.Lock()
			defer mu.Unlock()
			texts = append(texts, text)
		},
	})

	c.updateSegments([]Segment{{Start: 0, End: 1, Text: " hello world ", Completed: false}})

	mu.Lock()
	defer mu.Unlock()
	if len(texts) == 0 {
		t.Fatal("OnTranscription was not called")
	}
	if texts[0] != "hello world" {
		t.Errorf("text = %q, want %q", texts[0], "hello world")
	}
}

func TestUpdateSegmentsActivityTimestampUpdatesOnNewText(t *testing.T) {
	c := newTestClient(Config{})
	before := time.Now()
	time.Sleep(2 * time.Millisecond)

	c.updateSegments([]Segment{{Start: 0, End: 1, Text: "new text", Completed: false}})

	c.mu.Lock()
	activity := c.lastActivity
	c.mu.Unlock()

	if !activity.After(before) {
		t.Errorf("lastActivity not updated after new text")
	}
}

func TestUpdateSegmentsActivityTimestampNotUpdatedOnSameText(t *testing.T) {
	c := newTestClient(Config{})
	c.updateSegments([]Segment{{Start: 0, End: 1, Text: "same", Completed: false}})

	c.mu.Lock()
	first := c.lastActivity
	c.mu.Unlock()

	time.Sleep(2 * time.Millisecond)
	c.updateSegments([]Segment{{Start: 0, End: 1, Text: "same", Completed: false}})

	c.mu.Lock()
	second := c.lastActivity
	c.mu.Unlock()

	if second != first {
		t.Errorf("lastActivity changed on repeated identical text")
	}
}

func TestUpdateSegmentsDoesNotAppendOverlappingCompleted(t *testing.T) {
	c := newTestClient(Config{})
	c.updateSegments([]Segment{{Start: 0, End: 2.0, Text: "first", Completed: true}})
	// A completed segment that starts before the previous one ended must not be
	// appended, even though its text differs.
	c.updateSegments([]Segment{{Start: 1.0, End: 3.0, Text: "second", Completed: true}})

	n := len(c.transcript)
	if n != 1 {
		t.Errorf("transcript has %d segments, want 1 (overlapping segment should be skipped)", n)
	}
}

func TestUpdateSegmentsDoesNotAppendSameTextAfterGap(t *testing.T) {
	c := newTestClient(Config{})
	c.updateSegments([]Segment{{Start: 0, End: 1.0, Text: "repeat", Completed: true}})
	// Non-overlapping start but identical text: still must not be appended.
	c.updateSegments([]Segment{{Start: 1.0, End: 2.0, Text: "repeat", Completed: true}})

	n := len(c.transcript)
	if n != 1 {
		t.Errorf("transcript has %d segments, want 1 (identical text should be skipped)", n)
	}
}

func TestUpdateSegmentsAppendsMultipleCompletedInSingleBatch(t *testing.T) {
	c := newTestClient(Config{})
	c.updateSegments([]Segment{
		{Start: 0, End: 1.0, Text: "first", Completed: true},
		{Start: 1.0, End: 2.0, Text: "second", Completed: true},
	})

	n := len(c.transcript)
	if n != 2 {
		t.Errorf("transcript has %d segments, want 2", n)
	}
	if got := c.FinalTranscript(); got != "first second" {
		t.Errorf("got %q, want %q", got, "first second")
	}
}

func TestUpdateSegmentsCommitsWhenPartialBecomesCompleted(t *testing.T) {
	var mu sync.Mutex
	var commits []string
	c := newTestClient(Config{
		Callbacks: backends.BackendCallbacks{
			OnCommit: func(text string) {
				mu.Lock()
				defer mu.Unlock()
				commits = append(commits, text)
			},
		},
	})

	// The same single segment is first emitted as a partial, then completed.
	c.updateSegments([]Segment{{Start: 0, End: 1, Text: "hello", Completed: false}})
	c.updateSegments([]Segment{{Start: 0, End: 1, Text: "hello", Completed: true}})

	if len(commits) != 1 {
		t.Fatalf("got %d commit calls, want 1: %v", len(commits), commits)
	}
	if commits[0] != "hello" {
		t.Errorf("commit[0] = %q, want %q", commits[0], "hello")
	}
}

func TestUpdateSegmentsCommitsCompletedTextOnlyOnce(t *testing.T) {
	var mu sync.Mutex
	var commits []string
	c := newTestClient(Config{
		Callbacks: backends.BackendCallbacks{
			OnCommit: func(text string) {
				mu.Lock()
				defer mu.Unlock()
				commits = append(commits, text)
			},
		},
	})

	c.updateSegments([]Segment{{Start: 0, End: 1, Text: "hello", Completed: false}})
	c.updateSegments([]Segment{{Start: 0, End: 1, Text: "hello", Completed: true}})
	// The backend re-sends the same completed segment; it must not commit again.
	c.updateSegments([]Segment{{Start: 0, End: 1, Text: "hello", Completed: true}})

	if len(commits) != 1 {
		t.Errorf("got %d commit calls, want 1 (repeated completed segment should not re-commit): %v", len(commits), commits)
	}
}

func TestUpdateSegmentsNilCallbacksDoNotPanic(t *testing.T) {
	c := newTestClient(Config{}) // no callbacks configured

	// Exercise the delta, revision, commit and completion paths without panicking.
	c.updateSegments([]Segment{{Start: 0, End: 1, Text: "hello", Completed: false}})
	c.updateSegments([]Segment{{Start: 0, End: 1, Text: "hello world", Completed: false}})
	c.updateSegments([]Segment{{Start: 0, End: 1, Text: "rewritten", Completed: false}})
	c.updateSegments([]Segment{
		{Start: 0, End: 1, Text: "rewritten", Completed: true},
		{Start: 1, End: 2, Text: "next", Completed: false},
	})
}

func TestUpdateSegmentsNoDoubleCommitWhenArrayGrows(t *testing.T) {
	var mu sync.Mutex
	var commits []string
	c := newTestClient(Config{
		Callbacks: backends.BackendCallbacks{
			OnCommit: func(text string) {
				mu.Lock()
				defer mu.Unlock()
				commits = append(commits, text)
			},
		},
	})

	// Step 1: partial
	c.updateSegments([]Segment{{Start: 0, End: 1, Text: "hello", Completed: false}})

	// Step 2: same segment becomes completed → OnCommit("hello") must fire once.
	c.updateSegments([]Segment{{Start: 0, End: 1, Text: "hello", Completed: true}})

	// Step 3: server appends a new partial, growing the array.
	// newSegmentID becomes 1; without the fix the segmentID-change block would
	// re-commit segments[0].Text = "hello" a second time.
	c.updateSegments([]Segment{
		{Start: 0, End: 1, Text: "hello", Completed: true},
		{Start: 1, End: 2, Text: "world", Completed: false},
	})

	mu.Lock()
	defer mu.Unlock()

	nonEmpty := 0
	for _, cm := range commits {
		if cm != "" {
			nonEmpty++
		}
	}
	if nonEmpty != 1 {
		t.Errorf("got %d non-empty commit(s) %v, want exactly 1 (no double-commit on array growth)", nonEmpty, commits)
	}
}

func TestUpdateSegmentsPartialIsReplacedByNewerPartial(t *testing.T) {
	c := newTestClient(Config{})
	c.updateSegments([]Segment{{Start: 0, End: 1, Text: "hello", Completed: false}})
	c.updateSegments([]Segment{{Start: 0, End: 1, Text: "hello world", Completed: false}})

	partial := c.partial

	if partial == nil {
		t.Fatal("partial = nil, want the latest partial segment")
	}
	if partial.Text != "hello world" {
		t.Errorf("partial.Text = %q, want %q", partial.Text, "hello world")
	}
	if got := c.FinalTranscript(); got != "hello world" {
		t.Errorf("got %q, want %q", got, "hello world")
	}
}
