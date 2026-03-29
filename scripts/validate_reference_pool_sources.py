#!/usr/bin/env python3
from __future__ import annotations

import json
import pathlib
import sys
from collections import Counter

ROOT = pathlib.Path(__file__).resolve().parents[1]
REFERENCE_ROOTS = [
    ROOT / "assets/reference_pools",
]
LICENSED_ROOT = "assets/reference_intake/licensed_audio/"
HOME_MUSIC = str(pathlib.Path.home() / "Music").lower()
MIN_PER_GENRE = 100


def iter_manifests() -> list[pathlib.Path]:
    manifests: list[pathlib.Path] = []
    for base in REFERENCE_ROOTS:
        if not base.exists():
            continue
        manifests.extend(sorted(base.glob("*/pool_manifest.json")))
    return manifests


def main() -> int:
    failed = False
    for manifest_path in iter_manifests():
      try:
        payload = json.loads(manifest_path.read_text(encoding="utf-8"))
      except Exception as exc:
        print(f"{manifest_path}: invalid json: {exc}", file=sys.stderr)
        failed = True
        continue

      references = payload.get("references") or []
      counts: Counter[str] = Counter()
      for item in references:
        genre = str(item.get("genre") or "").strip().lower()
        source_path = str(item.get("source_path") or "").strip()
        if genre:
          counts[genre] += 1
        if HOME_MUSIC in source_path.lower():
          print(f"{manifest_path}: disallowed source path {source_path}", file=sys.stderr)
          failed = True
        if source_path and not source_path.startswith(LICENSED_ROOT):
          print(f"{manifest_path}: source outside licensed intake {source_path}", file=sys.stderr)
          failed = True

      if counts:
        for genre, count in sorted(counts.items()):
          if count < MIN_PER_GENRE:
            print(f"{manifest_path}: genre {genre} has {count}, needs {MIN_PER_GENRE}", file=sys.stderr)
            failed = True

    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
