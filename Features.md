# Features (Planned)

- Monitors a given user process
- User should be able to specify time intervals
    - either at random time duration, specific time duration, or periodic
- if a fault is detect will initiate recovery measure
    - faulty process info will be sent to deployment manager which will restart process
- logs shall include information like
    - Timestamp:
    ProcessName:
    Action: {ProcessStarted/FailureIntroduced/FailureDetected}
