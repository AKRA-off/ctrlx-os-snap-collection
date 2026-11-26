package events

import (
	"encoding/json"
	"fmt"
)

type EventBase struct {
	Type string `json:"type"`
}

type Event interface {
	New()
}

func FromJson(jsonData []byte) (Event, error) {
	var msg EventBase
	err := json.Unmarshal(jsonData, &msg)
	if err != nil {
		return nil, fmt.Errorf("unmarshaling JSON: %w", err)
	}

	var dst Event
	switch msg.Type {

	case "session.created":
		dst = new(SessionCreated)
	case "session.update":
		dst = new(SessionUpdate)
	case "session.updated":
		dst = new(SessionUpdated)
	case "model.loaded":
		dst = new(ModelLoaded)
	case "model.unloaded":
		dst = new(ModelUnloaded)
	case "input_audio_buffer.append":
		dst = new(InputAudioBufferAppend)
	case "input_audio_buffer.commit":
		dst = new(InputAudioBufferCommit)
	case "conversation.item.input_audio_transcription.delta":
		dst = new(ConversationItemInputAudioTranscriptionDelta)
	case "conversation.item.input_audio_transcription.completed":
		dst = new(ConversationItemInputAudioTranscriptionCompleted)
	case "error":
		dst = new(Error)
	default:
		return nil, fmt.Errorf("unknown event type: %s", msg.Type)
	}

	if err := json.Unmarshal(jsonData, dst); err != nil {
		return nil, fmt.Errorf("unmarshaling JSON: %w", err)
	}
	return dst, nil
}

// ToJson serializes an outbound event to its JSON wire form. It calls New() to
// guarantee the "type" discriminator is populated regardless of how the value
// was constructed.
func ToJson(m Event) ([]byte, error) {
	m.New()
	data, err := json.Marshal(m)
	if err != nil {
		return nil, fmt.Errorf("marshaling JSON: %w", err)
	}
	return data, nil
}
