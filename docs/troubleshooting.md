# Troubleshooting

## No audio output
- Check first-run audio device selection and buffer/sample-rate compatibility.

## VST3 plugins not appearing
- Add paths in first-run config or update `config/first_run.json`.

## Analysis files missing
- Verify `analysis/` is writable.

## Performance spikes
- Increase buffer size and disable advisory adapter.
