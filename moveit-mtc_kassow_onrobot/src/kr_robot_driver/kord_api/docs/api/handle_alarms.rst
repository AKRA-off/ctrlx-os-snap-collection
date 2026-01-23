Handling System Issues
======================

The KORD CBun monitors various system statistics to ensure safe and efficient robot operation. If any statistical thresholds are exceeded, the CBun will handle the situation as follows:

1. **Notification:** A pop-up or alarm dialog appears on the Teach Pendant, providing a brief description of the issue.

2. **Safety Trigger:**  If the `QOC_HALT_TRIGGER` feature is enabled and a threshold is exceeded, the robot movement is suspended automatically.
