package events

type DeltaLogprob struct {
	Token   string  `json:"token"`
	Logprob float64 `json:"logprob"`
	Bytes   []int   `json:"bytes,omitempty"`
}

type ConversationItemInputAudioTranscriptionDelta struct {
	EventBase
	Delta    string         `json:"delta"`
	Logprobs []DeltaLogprob `json:"logprobs,omitempty"`
}

func (m *ConversationItemInputAudioTranscriptionDelta) New() {
	m.Type = "conversation.item.input_audio_transcription.delta"
}

// NewTranscriptionDelta builds an incremental transcript fragment event.
func NewTranscriptionDelta(delta string) *ConversationItemInputAudioTranscriptionDelta {
	m := &ConversationItemInputAudioTranscriptionDelta{
		Delta: delta,
	}
	m.New()
	return m
}
