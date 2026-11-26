package events

type InputAudioBufferCommit struct {
	EventBase
}

func (m *InputAudioBufferCommit) New() {
	m.Type = "input_audio_buffer.commit"
}
