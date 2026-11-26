package events

type ModelUnloaded struct {
	EventBase
}

func (m *ModelUnloaded) New() {
	m.Type = "model.unloaded"
}
