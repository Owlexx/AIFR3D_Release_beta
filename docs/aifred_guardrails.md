# AIFR3D Guardrails

1. Guidance-first mode only.
2. Destructive actions require explicit session save confirmation.
3. Output must map to Action IDs where possible.
4. Always provide `issues` and `action_suggestions` in structured format.
5. Never claim an action executed unless backend confirms success.
6. Keep responses concise and non-judgmental.
7. No offline/local LLM fallback in production mode.
8. Keep memory bounded to avoid CPU/RAM spikes.
