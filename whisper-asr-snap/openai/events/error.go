package events

type ErrorDetail struct {
	Type    string `json:"type"`
	Code    string `json:"code"`
	Message string `json:"message"`
}

type Error struct {
	EventBase
	Error ErrorDetail `json:"error"`
}

func (m *Error) New() {
	m.Type = "error"
}

// Error type / code constants as defined by the Myna error model.
const (
	ErrorTypeInvalidRequest = "invalid_request_error"
	ErrorTypeServer         = "server_error"

	ErrorCodeUnknownParameter = "unknown_parameter"
	ErrorCodeInvalidParameter = "invalid_parameter"
	ErrorCodeServerError      = "server_error"
	ErrorCodeNoModelError     = "no_model_error"
)

// NewError builds a structured error event.
func NewError(errType, code, message string) *Error {
	m := &Error{
		Error: ErrorDetail{
			Type:    errType,
			Code:    code,
			Message: message,
		},
	}
	m.New()
	return m
}
