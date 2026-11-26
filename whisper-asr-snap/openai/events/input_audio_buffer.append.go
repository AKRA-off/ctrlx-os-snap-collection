package events

type InputAudioBufferAppend struct {
	EventBase
	Audio string `json:"audio"`
}

func (m *InputAudioBufferAppend) New() {
	m.Type = "input_audio_buffer.append"
}
