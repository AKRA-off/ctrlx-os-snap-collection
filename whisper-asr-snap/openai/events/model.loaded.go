package events

type ModelLoaded struct {
	EventBase
}

func (m *ModelLoaded) New() {
	m.Type = "model.loaded"
}
