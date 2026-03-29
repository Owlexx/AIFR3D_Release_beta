# Licensed Reference Pool Workflow (v2.2.4)

## Purpose

Use this path when you have purchased or otherwise licensed music locally and want to profile it into a lawful reference pool.

## Runtime Contract

- Live desktop meters compare the current mix against the nearest individual reference track.
- Advisory target windows use the detected genre mean with a fixed `±3` tolerance.
- Raw licensed audio stays local and is not tracked in git.

## Folder Layout

Put licensed audio under:

- `assets/reference_intake/licensed_audio/rap`
- `assets/reference_intake/licensed_audio/hip-hop`
- `assets/reference_intake/licensed_audio/edm`
- `assets/reference_intake/licensed_audio/dubstep`
- `assets/reference_intake/licensed_audio/pop`
- `assets/reference_intake/licensed_audio/rock`

## Config

Start from:

- `assets/reference_intake/licensed_pool_config.template.json`

Update `count_target` or `input_dir` values as needed.

## Build

```bash
python3 scripts/build_licensed_reference_pool.py \
  --config assets/reference_intake/licensed_pool_config.template.json
```

Optional explicit CLI path:

```bash
python3 scripts/build_licensed_reference_pool.py \
  --dawai-bin build/linux-debug/apps/dawai/dawai_artefacts/Debug/DawAI
```

## Output

The script writes mirrored pools to:

- `assets/reference_pools/<pool_id>`
- `assets/reference_pools/<pool_id>`

Each generated pool includes:

- `pool.json`
- `pool_manifest.json`
- `selection_report.json`
- `profiles/*.json`

## Selection Logic

- Every source track is profiled individually.
- Genre targets are derived from the arithmetic mean of accepted reference tracks in that genre.
- Tracks are kept only when they remain within `±3` of the genre target metrics configured in the file.

## Notes

- This workflow does not download or scrape copyrighted audio.
- If you want stricter intake, set `"strict_count": true` per genre.
