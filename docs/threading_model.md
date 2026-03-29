# Threading Model

## Threads

- Audio thread: pull-based callback, transport/track render, meter tap write only.
- Analysis worker: FFT windows, profile updates, scoring/fixlist generation.
- Advisory worker: rule/LLM rewrite of deterministic outputs.
- UI thread: rendering and user interactions.

## Rules

- No dynamic allocation in tight audio callback loops where avoidable.
- No blocking locks on audio thread.
- Ring buffers are used for meter snapshots.
- Advisory calls are throttled and cancellable.
