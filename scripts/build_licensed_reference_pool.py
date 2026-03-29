#!/usr/bin/env python3
"""Build a licensed reference pool from copyright-cleared professional releases.

This script is for manual, lawful intake only.
You point it at folders of licensed or copyright-cleared released audio, grouped by genre, and it:

1. Uses the built `DawAI` CLI to profile each genre folder.
2. Derives or applies target metrics per genre.
3. Keeps only tracks within the requested tolerance window.
4. Writes canonical pool artifacts to:
   - `assets/reference_pools/<pool_id>`
"""

from __future__ import annotations

import argparse
import json
import pathlib
import shutil
import statistics
import subprocess
import tempfile
from collections import Counter
from datetime import datetime, timezone

ROOT = pathlib.Path(__file__).resolve().parents[1]
OUTPUT_ROOTS = [ROOT / "assets/reference_pools"]
DEFAULT_TOLERANCES = {
    "integrated_lufs": 3.0,
    "true_peak_dbtp": 3.0,
    "crest_factor_db": 3.0,
    "band_average_db": 3.0,
}


def clean_text(value: object) -> str:
    return str(value or "").strip()


def slugify(value: str) -> str:
    out: list[str] = []
    previous_dash = False
    for char in clean_text(value).lower():
        if char.isalnum():
            out.append(char)
            previous_dash = False
        elif not previous_dash:
            out.append("-")
            previous_dash = True
    while out and out[0] == "-":
        out.pop(0)
    while out and out[-1] == "-":
        out.pop()
    return "".join(out) or "pool"


def load_json(path: pathlib.Path) -> object:
    return json.loads(path.read_text(encoding="utf-8"))


def write_json(path: pathlib.Path, payload: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def load_metric_catalog(config: dict) -> dict:
    raw = clean_text(config.get("metric_catalog") or (ROOT / "assets/reference_intake/analysis_metric_catalog.json"))
    catalog_path = pathlib.Path(raw).expanduser()
    if not catalog_path.is_absolute():
        catalog_path = ROOT / catalog_path
    if not catalog_path.exists():
        return {}
    payload = load_json(catalog_path)
    return dict(payload.get("metrics") or {}) if isinstance(payload, dict) else {}


def normalize_genre(raw: str, aliases: dict[str, str]) -> str:
    genre = clean_text(raw).lower()
    if genre in {"hiphop", "hip hop"}:
        genre = "hip-hop"
    return aliases.get(genre, genre)


def discover_dawai_bin(explicit_path: str) -> pathlib.Path:
    candidates: list[pathlib.Path] = []
    if explicit_path:
        candidates.append(pathlib.Path(explicit_path).expanduser())
    env_path = clean_text(pathlib.os.environ.get("DAWAI_BIN", ""))
    if env_path:
        candidates.append(pathlib.Path(env_path).expanduser())

    candidates.extend(
        [
            ROOT / "build/linux-debug/apps/dawai/dawai_artefacts/Debug/DawAI",
            ROOT / "build/apps/dawai/dawai_artefacts/Debug/DawAI",
            ROOT / "build/apps/dawai/dawai_artefacts/Release/DawAI",
            ROOT / "build/linux-debug/apps/dawai/dawai_artefacts/Release/DawAI",
        ]
    )

    which_path = shutil.which("DawAI")
    if which_path:
        candidates.append(pathlib.Path(which_path))

    for candidate in candidates:
        if candidate.exists():
            return candidate

    raise SystemExit(
        "Could not find `DawAI` CLI. Build the repo first or pass --dawai-bin /path/to/DawAI."
    )


def crest_factor_db(profile: dict) -> float | None:
    values = profile.get("crestFactorDistribution", [])
    if isinstance(values, list) and values:
        try:
            return float(values[0])
        except (TypeError, ValueError):
            return None
    return None


def spectrum_bands(profile: dict) -> list[float]:
    values = profile.get("spectrumBandsDb", [])
    if not isinstance(values, list):
        return []
    out: list[float] = []
    for value in values:
        try:
            out.append(float(value))
        except (TypeError, ValueError):
            out.append(-120.0)
    return out


def average(values: list[float], fallback: float = 0.0) -> float:
    if not values:
        return fallback
    return float(statistics.fmean(values))


def derive_targets(profiles: list[dict]) -> dict:
    bands = [spectrum_bands(profile) for profile in profiles]
    band_count = max((len(item) for item in bands), default=0)
    target_bands: list[float] = []
    for index in range(band_count):
        samples = [item[index] for item in bands if index < len(item)]
        target_bands.append(average(samples, -120.0))

    true_peak_values = []
    for profile in profiles:
        value = profile.get("truePeakDbtp", profile.get("true_peak_dbtp", -1.0))
        try:
            parsed = float(value)
        except (TypeError, ValueError):
            continue
        if -80.0 < parsed <= 3.0:
            true_peak_values.append(parsed)

    crest_values = [value for value in (crest_factor_db(profile) for profile in profiles) if value is not None]

    return {
        "integrated_lufs": average(
            [float(profile.get("integratedLufs", -23.0)) for profile in profiles],
            -23.0,
        ),
        "true_peak_dbtp": average(true_peak_values, -1.0),
        "crest_factor_db": average(crest_values, 10.0),
        "spectrum_bands_db": target_bands,
    }


def compute_deviation(profile: dict, targets: dict) -> tuple[dict[str, float], float]:
    crest = crest_factor_db(profile)
    profile_bands = spectrum_bands(profile)
    target_bands = targets.get("spectrum_bands_db", [])

    band_samples = []
    for index in range(min(len(profile_bands), len(target_bands))):
        band_samples.append(abs(profile_bands[index] - float(target_bands[index])))

    deviations = {
        "integrated_lufs": abs(float(profile.get("integratedLufs", -23.0)) - float(targets["integrated_lufs"])),
        "true_peak_dbtp": abs(
            float(profile.get("truePeakDbtp", profile.get("true_peak_dbtp", -1.0)))
            - float(targets["true_peak_dbtp"])
        ),
        "crest_factor_db": abs((crest if crest is not None else float(targets["crest_factor_db"])) - float(targets["crest_factor_db"])),
        "band_average_db": (sum(band_samples) / len(band_samples)) if band_samples else 0.0,
    }
    return deviations, sum(deviations.values())


def within_tolerance(deviations: dict[str, float], tolerances: dict[str, float]) -> bool:
    for key, limit in tolerances.items():
        if deviations.get(key, 0.0) > float(limit):
            return False
    return True


def load_profile(path: pathlib.Path) -> dict:
    payload = load_json(path)
    if isinstance(payload, list):
        return dict(payload[0]) if payload else {}
    return dict(payload)


def run_pool_build(
    dawai_bin: pathlib.Path,
    input_dir: pathlib.Path,
    output_dir: pathlib.Path,
    pool_name: str,
    genre: str,
) -> None:
    command = [
        str(dawai_bin),
        "--build-reference-pool",
        str(input_dir),
        "--out",
        str(output_dir),
        "--pool-name",
        pool_name,
        "--genre",
        genre,
        "--max-files",
        "0",
    ]
    subprocess.run(command, check=True)


def build_genre_selection(
    dawai_bin: pathlib.Path,
    workdir: pathlib.Path,
    genre_cfg: dict,
    tolerances: dict[str, float],
    aliases: dict[str, str],
) -> dict:
    raw_genre = clean_text(genre_cfg.get("genre"))
    genre = normalize_genre(raw_genre, aliases)
    input_dir = pathlib.Path(clean_text(genre_cfg.get("input_dir"))).expanduser()
    if not input_dir.exists():
        raise SystemExit(f"Input directory missing for genre `{genre}`: {input_dir}")

    genre_workdir = workdir / slugify(genre)
    run_pool_build(
        dawai_bin=dawai_bin,
        input_dir=input_dir,
        output_dir=genre_workdir,
        pool_name=clean_text(genre_cfg.get("pool_name")) or f"Licensed {genre.title()} Pool",
        genre=genre,
    )

    profile_dir = genre_workdir / "profiles"
    profile_files = sorted(profile_dir.glob("*.json"))
    profiles = [load_profile(path) for path in profile_files]
    if not profiles:
        raise SystemExit(f"No profiles were created for genre `{genre}` from {input_dir}")

    targets = dict(genre_cfg.get("target_metrics") or derive_targets(profiles))
    target_count = int(genre_cfg.get("count_target", 0) or 0)
    strict_count = bool(genre_cfg.get("strict_count", False))

    accepted: list[dict] = []
    rejected: list[dict] = []
    for path, profile in zip(profile_files, profiles, strict=True):
        deviations, total = compute_deviation(profile, targets)
        candidate = {
            "path": path,
            "profile": profile,
            "deviations": deviations,
            "total_deviation": total,
        }
        if within_tolerance(deviations, tolerances):
            accepted.append(candidate)
        else:
            rejected.append(candidate)

    accepted.sort(key=lambda item: (item["total_deviation"], clean_text(item["profile"].get("title"))))
    selected = accepted[:target_count] if target_count > 0 else accepted

    if strict_count and target_count > 0 and len(selected) < target_count:
        raise SystemExit(
            f"Genre `{genre}` only produced {len(selected)} accepted profiles; target was {target_count}."
        )

    return {
        "genre": genre,
        "input_dir": str(input_dir),
        "target_count": target_count,
        "accepted_count": len(accepted),
        "selected_count": len(selected),
        "targets": targets,
        "selected": selected,
        "rejected_count": len(rejected),
    }


def write_outputs(pool_id: str, pool_name: str, selections: list[dict], tolerances: dict[str, float], metric_catalog: dict) -> None:
    counts = Counter()
    manifest_refs: list[dict] = []

    for root in OUTPUT_ROOTS:
        output_root = root / pool_id
        profiles_dir = output_root / "profiles"
        profiles_dir.mkdir(parents=True, exist_ok=True)
        for stale in profiles_dir.glob("*.json"):
            stale.unlink()

        pool_entries = []
        selection_report = {
            "pool_id": pool_id,
            "pool_name": pool_name,
            "tolerances": tolerances,
            "metrics": metric_catalog,
            "genres": [],
        }

        for selection in selections:
            genre = selection["genre"]
            counts[genre] += selection["selected_count"]
            selection_report["genres"].append(
                {
                    "genre": genre,
                    "input_dir": selection["input_dir"],
                    "target_count": selection["target_count"],
                    "accepted_count": selection["accepted_count"],
                    "selected_count": selection["selected_count"],
                    "rejected_count": selection["rejected_count"],
                    "targets": selection["targets"],
                }
            )

            for candidate in selection["selected"]:
                profile = dict(candidate["profile"])
                profile["genre"] = genre
                profile_name = pathlib.Path(candidate["path"]).name
                profile_rel = f"profiles/{profile_name}"
                write_json(profiles_dir / profile_name, [profile])
                ref_id = clean_text(profile.get("id")) or pathlib.Path(profile_name).stem
                pool_entries.append({"profileId": ref_id, "profilePath": profile_rel})
                manifest_refs.append(
                    {
                        "reference_id": ref_id,
                        "genre": genre,
                        "title": clean_text(profile.get("title")) or ref_id,
                        "source_path": clean_text(profile.get("sourcePath")),
                        "profile_path": profile_rel,
                        "integrated_lufs": float(profile.get("integratedLufs", -23.0)),
                        "short_term_lufs": float(profile.get("shortTermLufs", profile.get("short_term_lufs", -23.0))),
                        "true_peak_dbtp": float(profile.get("truePeakDbtp", profile.get("true_peak_dbtp", -1.0))),
                        "rms_db": float(profile.get("rmsDb", profile.get("rms_db", -70.0))),
                        "peak_dbfs": float(profile.get("peakDbfs", profile.get("peak_dbfs", -120.0))),
                        "crest_factor_db": crest_factor_db(profile),
                        "spectral_tilt_db": float(profile.get("spectralTiltDb", profile.get("spectralTilt", 0.0))),
                        "low_band_db": float(profile.get("lowBandDb", -90.0)),
                        "low_mid_band_db": float(profile.get("lowMidBandDb", -90.0)),
                        "presence_band_db": float(profile.get("presenceBandDb", -90.0)),
                        "air_band_db": float(profile.get("airBandDb", -90.0)),
                        "correlation": float(profile.get("correlation", 1.0)),
                        "stereo_width": float(profile.get("stereoWidth", 0.0)),
                        "mid_energy": float(profile.get("midEnergy", 0.0)),
                        "side_energy": float(profile.get("sideEnergy", 0.0)),
                        "phase_risk": float(profile.get("phaseRisk", 0.0)),
                        "sub_mono_integrity": float(profile.get("subMonoIntegrity", 1.0)),
                        "transient_density": float(profile.get("transientDensity", 0.0)),
                    }
                )

        pool_json = {"name": pool_name, "genre": "multi", "entries": pool_entries}
        manifest = {
            "schema_version": "refpool-2.2.4",
            "pool_id": pool_id,
            "created_utc": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
            "genres": sorted(counts),
            "counts": {
                "total_references": len(manifest_refs),
                "by_genre": dict(sorted(counts.items())),
            },
            "tolerances": tolerances,
            "metrics": metric_catalog,
            "references": manifest_refs,
        }

        write_json(output_root / "pool.json", pool_json)
        write_json(output_root / "pool_manifest.json", manifest)
        write_json(output_root / "selection_report.json", selection_report)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--config",
        default=str(ROOT / "assets/reference_intake/licensed_pool_config.template.json"),
        help="Path to licensed pool config JSON.",
    )
    parser.add_argument("--pool-id", default="", help="Override output pool id.")
    parser.add_argument("--pool-name", default="", help="Override output pool name.")
    parser.add_argument("--dawai-bin", default="", help="Path to built DawAI CLI.")
    parser.add_argument(
        "--keep-workdir",
        action="store_true",
        help="Keep intermediate per-genre build outputs for inspection.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    config_path = pathlib.Path(args.config).expanduser()
    config = load_json(config_path)
    if not isinstance(config, dict):
        raise SystemExit(f"Invalid config object: {config_path}")

    pool_id = clean_text(args.pool_id or config.get("pool_id")) or "licensed_reference_pool"
    pool_name = clean_text(args.pool_name or config.get("pool_name")) or "Licensed Reference Pool"
    tolerances = dict(DEFAULT_TOLERANCES)
    tolerances.update(config.get("tolerances") or {})
    aliases = {clean_text(k).lower(): clean_text(v).lower() for k, v in (config.get("genre_aliases") or {}).items()}
    metric_catalog = load_metric_catalog(config)
    genres = config.get("genres") or []
    if not isinstance(genres, list) or not genres:
        raise SystemExit(f"Config has no genres: {config_path}")

    dawai_bin = discover_dawai_bin(args.dawai_bin)
    workdir_path = pathlib.Path(tempfile.mkdtemp(prefix=f"{slugify(pool_id)}_", dir="/tmp"))

    try:
        selections = [
            build_genre_selection(
                dawai_bin=dawai_bin,
                workdir=workdir_path,
                genre_cfg=dict(item),
                tolerances=tolerances,
                aliases=aliases,
            )
            for item in genres
        ]
        write_outputs(pool_id=pool_id, pool_name=pool_name, selections=selections, tolerances=tolerances, metric_catalog=metric_catalog)
    finally:
        if args.keep_workdir:
            print(f"Kept workdir: {workdir_path}")
        else:
            shutil.rmtree(workdir_path, ignore_errors=True)

    print(f"Built licensed pool `{pool_id}`")
    for selection in selections:
        print(
            f"- {selection['genre']}: selected {selection['selected_count']} / "
            f"accepted {selection['accepted_count']} / target {selection['target_count']}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
