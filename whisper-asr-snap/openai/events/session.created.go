package events

import "myna-adapter/backends"

type SessionCreated struct {
	EventBase
	Session SessionData `json:"session"`
}

func (m *SessionCreated) New() {
	m.Type = "session.created"
}

func NewSessionCreated(cfg *backends.SessionConfig) *SessionCreated {
	m := &SessionCreated{
		Session: SessionData{
			Type:         &cfg.Type,
			Instructions: cfg.Instructions,
			Prompt:       cfg.Prompt,
			Include:      cfg.Include,
			Audio: &SessionAudio{
				Input: &SessionAudioInput{
					Format: &SessionAudioFormat{Rate: cfg.SampleRate},
					Transcription: &SessionTranscription{
						Model:    &cfg.Model,
						Language: &cfg.Lang,
					},
				},
			},
		},
	}
	m.New()
	return m
}
