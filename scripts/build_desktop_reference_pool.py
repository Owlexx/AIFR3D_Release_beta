#!/usr/bin/env python3
"""Build curated desktop reference pool with explicit genre tags.

Generates a deterministic 25-track pool spanning:
- rap
- hip-hop
- edm
- dubstep
- pop
- rock

Outputs canonical pool artifacts under:
- assets/reference_pools/pro_25x6
"""

from __future__ import annotations

import json
import pathlib
from collections import Counter
from dataclasses import dataclass
from datetime import datetime, timezone

ROOT = pathlib.Path(__file__).resolve().parents[1]
ARCHIVE_PROFILES = ROOT / "assets/reference_pools/canonical_25x5/profiles"
METRIC_CATALOG = ROOT / "assets/reference_intake/analysis_metric_catalog.json"

OUTPUT_ROOTS = [ROOT / "assets/reference_pools/pro_25x6"]


@dataclass(frozen=True)
class Selection:
    genre: str
    file_name: str


# Curated deterministic selection (25 total, 6 genres).
SELECTIONS: list[Selection] = [
    # rap (5)
    Selection("rap", "ref_31_380motion-thisiswhatido-124-380-motion-this-is-what-i-do.json"),
    Selection("rap", "ref_32_cassandrasgame-cassandra-s-game.json"),
    Selection("rap", "ref_33_dwandmianimalcellrap-dw-and-mi-animal-cell-rap.json"),
    Selection("rap", "ref_34_haveahgoodtime-have-ah-good-time.json"),
    Selection(
        "rap",
        "ref_35_interviewmitdiedrichdietrichsenuberamerikanischenrapundbrdpunk-interview-mit-diedrich-dietrichsen-uber-amerikanischen-rap-und-brd-pun.json",
    ),
    # hip-hop (4)
    Selection("hip-hop", "ref_11_djiconsgangstabaybeez6-gangstabaybeez6.json"),
    Selection(
        "hip-hop",
        "ref_12_dreamlogicc-wuvstowies-dreamlogicc-wuv-stowies-live-middle-gallery-april-25-2009.json",
    ),
    Selection("hip-hop", "ref_13_easynite-easy-nite.json"),
    Selection("hip-hop", "ref_14_haveahgoodtime-have-ah-good-time.json"),
    # edm (4)
    Selection("edm", "ref_10_bestnewelectrohouse2016march-best-new-electro-house-2016-march.json"),
    Selection("edm", "ref_1_01-cowfish-2-junebug-dnb-set-live-at-cowfish.json"),
    Selection("edm", "ref_2_01-turkey-day-cookin-2023-11-23-turkey-day-dnb-2023.json"),
    Selection("edm", "ref_3_10-20210927-set-album.json"),
    # dubstep (4) sourced from trap/bass-heavy references
    Selection("dubstep", "ref_41_audiotrap-audio-trap.json"),
    Selection("dubstep", "ref_44_fox72z-808-trap-synth-fox72z-808-trap-synth.json"),
    Selection("dubstep", "ref_46_introducingcloudcitytrap-introducing-cloud-city-trap.json"),
    Selection("dubstep", "ref_49_trapbeat-201804-trap-beat-trap-instrumental-base-rap-trap-type-beat.json"),
    # pop (4)
    Selection("pop", "ref_21_10moresongsbythemoot-10-more-songs-by-the-moot.json"),
    Selection("pop", "ref_22_2010-03-05liveatthejukebox-2010-03-05-live-at-the-jukebox.json"),
    Selection("pop", "ref_25_3feetuplive-3-feet-up-live.json"),
    Selection("pop", "ref_27_630-brianna-tam-i-am-202308-brianna-tam-i-am.json"),
    # rock (4)
    Selection("rock", "ref_6_3rd-strike-rock-april-2025-earth-day-show-3rd-strike-rock-april-2025-earth-day-show.json"),
    Selection("rock", "ref_7_566770-duocore.json"),
    Selection(
        "rock",
        "ref_8_anythingbutbrokelivespringboardsouth2015-201507-anything-but-broke-live-springboard-south-2015.json",
    ),
    Selection("rock", "ref_29_abirdseldomseen-a-bird-seldom-seen.json"),
]


def load_profile(path: pathlib.Path) -> dict:
    payload = json.loads(path.read_text(encoding="utf-8"))
    if isinstance(payload, list):
        if not payload:
            raise ValueError(f"Empty profile array: {path}")
        obj = payload[0]
    elif isinstance(payload, dict):
        obj = payload
    else:
        raise ValueError(f"Unsupported profile payload: {path}")
    if not isinstance(obj, dict):
        raise ValueError(f"Profile entry is not object: {path}")
    return obj


def write_profile(path: pathlib.Path, obj: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps([obj], indent=2), encoding="utf-8")


def load_metric_catalog() -> dict:
    if not METRIC_CATALOG.exists():
        return {}
    payload = json.loads(METRIC_CATALOG.read_text(encoding="utf-8"))
    return dict(payload.get("metrics") or {}) if isinstance(payload, dict) else {}


def main() -> int:
    if not ARCHIVE_PROFILES.exists():
        raise SystemExit(f"Archive profile folder missing: {ARCHIVE_PROFILES}")

    by_genre = Counter()
    manifest_refs: list[dict] = []

    profile_objects: list[tuple[Selection, dict]] = []
    for pick in SELECTIONS:
        src = ARCHIVE_PROFILES / pick.file_name
        if not src.exists():
            raise SystemExit(f"Missing selected profile: {src}")
        profile = load_profile(src)
        profile["genre"] = pick.genre
        profile_objects.append((pick, profile))
        by_genre[pick.genre] += 1

    if len(profile_objects) != 25:
        raise SystemExit(f"Expected 25 selected references, got {len(profile_objects)}")

    pool_entries = []
    for pick, profile in profile_objects:
        ref_id = str(profile.get("id", pathlib.Path(pick.file_name).stem))
        profile_rel = f"profiles/{pick.file_name}"
        pool_entries.append({"profileId": ref_id, "profilePath": profile_rel})

        crest = None
        crest_list = profile.get("crestFactorDistribution", [])
        if isinstance(crest_list, list) and crest_list:
            try:
                crest = float(crest_list[0])
            except Exception:
                crest = None

        manifest_refs.append(
            {
                "reference_id": ref_id,
                "genre": pick.genre,
                "title": str(profile.get("title", ref_id)),
                "source_path": str(profile.get("sourcePath", "")),
                "profile_path": profile_rel,
                "sample_rate": None,
                "duration_seconds": None,
                "integrated_lufs": float(profile.get("integratedLufs", -23.0)),
                "short_term_lufs": float(profile.get("shortTermLufs", profile.get("short_term_lufs", -23.0))),
                "rms_db": float(profile.get("rmsDb", profile.get("rms_db", -70.0))),
                "peak_dbfs": float(profile.get("peakDbfs", profile.get("peak_dbfs", -120.0))),
                "crest_factor_db": crest,
                "true_peak_dbtp": float(profile.get("truePeakDbtp", profile.get("true_peak_dbtp", -1.0))),
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

    pool_json = {
        "name": "Pro Reference Pool 25x6",
        "genre": "multi",
        "entries": pool_entries,
    }

    manifest = {
        "schema_version": "refpool-2.2.4",
        "pool_id": "pro_25x6",
        "created_utc": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "genres": ["rap", "hip-hop", "edm", "dubstep", "pop", "rock"],
        "metrics": load_metric_catalog(),
        "counts": {
            "total_references": len(profile_objects),
            "by_genre": dict(by_genre),
        },
        "references": manifest_refs,
    }

    for root in OUTPUT_ROOTS:
        profiles_dir = root / "profiles"
        profiles_dir.mkdir(parents=True, exist_ok=True)

        # Clean only prior generated JSONs in this output pool.
        for stale in profiles_dir.glob("*.json"):
            stale.unlink()

        for pick, profile in profile_objects:
            write_profile(profiles_dir / pick.file_name, profile)

        (root / "pool.json").write_text(json.dumps(pool_json, indent=2), encoding="utf-8")
        (root / "pool_manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")

    print("Generated pro_25x6 reference pools:")
    for root in OUTPUT_ROOTS:
        print(f"- {root}")
    print("Genre counts:", dict(by_genre))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
