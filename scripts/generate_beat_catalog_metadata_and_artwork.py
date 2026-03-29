#!/usr/bin/env python3
"""Generate Beat Catalog metadata and per-track artwork variants.

This script analyzes each track for BPM/key/genre and creates a brand-based
artwork image variant derived from the website hero image.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import re
import subprocess
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterable
from urllib.parse import quote

import librosa
import numpy as np
from PIL import Image, ImageDraw, ImageEnhance, ImageFilter, ImageFont, ImageOps

AUDIO_EXTENSIONS = {".wav", ".mp3", ".flac", ".m4a", ".aiff", ".ogg"}
WEBSITE_AUDIO_BASE_URL = "/assets/audio/catalog"
MAX_WEBSITE_ASSET_BYTES = 24 * 1024 * 1024
DEFAULT_SOURCE_DIR_CANDIDATES = [
    os.environ.get("DAWAI_EXTERNAL_CATALOG_DIR", "").strip(),
    str(Path.home() / "Music/North3rnlight3r_Beatz"),
    str(Path.home() / "Music/North3rnlighter beats"),
    "/home/north3rnlight3r/Music/North3rnlight3r_Beatz",
    "/home/north3rnlight3r/Music/North3rnlighter beats",
]
TONICS = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
MAJOR_PROFILE = np.asarray([6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88], dtype=np.float32)
MINOR_PROFILE = np.asarray([6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17], dtype=np.float32)
BASE_COLORS = [
    (44, 210, 248),
    (68, 247, 200),
    (28, 166, 255),
    (64, 188, 255),
    (88, 227, 248),
]


def resolve_source_dir(preferred: str | None = None) -> str:
    candidates: list[str] = []
    if preferred:
        candidates.append(str(preferred).strip())
    candidates.extend(DEFAULT_SOURCE_DIR_CANDIDATES)
    seen: set[str] = set()
    for candidate in candidates:
        normalized = str(candidate or "").strip()
        if not normalized or normalized in seen:
            continue
        seen.add(normalized)
        path = Path(normalized).expanduser()
        if path.is_dir():
            return str(path)
    return str(Path.home() / "Music/North3rnlight3r_Beatz")


def list_audio_files(directory: Path) -> list[Path]:
    return sorted(
        [p for p in directory.iterdir() if p.is_file() and p.suffix.lower() in AUDIO_EXTENSIONS],
        key=lambda p: p.name.lower(),
    )


def slugify(value: str) -> str:
    value = re.sub(r"\.[^.]+$", "", value.strip())
    value = re.sub(r"[^a-zA-Z0-9]+", "-", value)
    value = value.strip("-").lower()
    return value or "track"


def safe_iso_utc(ts: float) -> str:
    return datetime.fromtimestamp(ts, tz=timezone.utc).isoformat().replace("+00:00", "Z")


def format_duration(seconds: float) -> str:
    if not np.isfinite(seconds) or seconds <= 0:
        return "00:00"
    total = int(round(seconds))
    mins = total // 60
    secs = total % 60
    return f"{mins:02d}:{secs:02d}"


def probe_duration_seconds(path: Path) -> float:
    try:
        probe = subprocess.run(
            [
                "ffprobe",
                "-v",
                "error",
                "-show_entries",
                "format=duration",
                "-of",
                "default=noprint_wrappers=1:nokey=1",
                str(path),
            ],
            check=False,
            capture_output=True,
            timeout=25,
            text=True,
        )
        if probe.returncode != 0:
            return 0.0
        value = float((probe.stdout or "").strip() or "0")
        return value if np.isfinite(value) and value > 0 else 0.0
    except Exception:
        return 0.0


def _normalized_corr(a: np.ndarray, b: np.ndarray) -> float:
    av = a - np.mean(a)
    bv = b - np.mean(b)
    denom = np.linalg.norm(av) * np.linalg.norm(bv)
    if denom <= 1e-8:
        return 0.0
    return float(np.dot(av, bv) / denom)


def estimate_key(chroma_mean: np.ndarray) -> str:
    best_score = -math.inf
    best_tonic = 0
    best_mode = "major"

    for shift in range(12):
        major_score = _normalized_corr(chroma_mean, np.roll(MAJOR_PROFILE, shift))
        if major_score > best_score:
            best_score = major_score
            best_tonic = shift
            best_mode = "major"

        minor_score = _normalized_corr(chroma_mean, np.roll(MINOR_PROFILE, shift))
        if minor_score > best_score:
            best_score = minor_score
            best_tonic = shift
            best_mode = "minor"

    return f"{TONICS[best_tonic]} {best_mode}"


def infer_genre(tempo: float, centroid: float, rolloff: float, zcr: float) -> str:
    if tempo >= 150:
        return "Drill / Trap"
    if tempo >= 132 and centroid >= 2300:
        return "Trap"
    if tempo >= 120 and rolloff >= 3500:
        return "Electronic Hip-Hop"
    if tempo <= 96 and zcr <= 0.08:
        return "Boom Bap"
    if tempo <= 110:
        return "Hip-Hop"
    return "Hip-Hop / Trap"


def analyze_track(path: Path) -> dict[str, str]:
    try:
        decode = subprocess.run(
            [
                "ffmpeg",
                "-v",
                "error",
                "-t",
                "120",
                "-i",
                str(path),
                "-f",
                "f32le",
                "-ac",
                "1",
                "-ar",
                "22050",
                "pipe:1",
            ],
            check=False,
            capture_output=True,
            timeout=45,
        )
        if decode.returncode != 0 or not decode.stdout:
            raise RuntimeError(decode.stderr.decode("utf-8", errors="ignore").strip() or "ffmpeg decode failed")
        y = np.frombuffer(decode.stdout, dtype=np.float32)
        sr = 22050
    except Exception as exc:  # pragma: no cover - defensive fallback for rare codec issues
        print(f"[warn] analysis fallback for {path.name}: {exc}")
        return {"bpm": "N/A", "tempo": "N/A", "key_signature": "N/A", "genre": "Hip-Hop"}
    if y.size == 0:
        return {"bpm": "N/A", "tempo": "N/A", "key_signature": "N/A", "genre": "Hip-Hop"}

    tempo_estimate, _ = librosa.beat.beat_track(y=y, sr=sr)
    tempo_value = float(np.atleast_1d(tempo_estimate)[0]) if tempo_estimate is not None else 0.0
    if not np.isfinite(tempo_value) or tempo_value <= 0:
        tempo_value = 0.0

    chroma = librosa.feature.chroma_cqt(y=y, sr=sr)
    chroma_mean = np.mean(chroma, axis=1) if chroma.size else np.zeros(12, dtype=np.float32)
    key_signature = estimate_key(chroma_mean)

    centroid = float(np.mean(librosa.feature.spectral_centroid(y=y, sr=sr)))
    rolloff = float(np.mean(librosa.feature.spectral_rolloff(y=y, sr=sr)))
    zcr = float(np.mean(librosa.feature.zero_crossing_rate(y)))

    bpm_int = int(round(np.clip(tempo_value, 60, 220))) if tempo_value > 0 else 0
    bpm = str(bpm_int) if bpm_int > 0 else "N/A"
    return {
        "bpm": bpm,
        "tempo": bpm,
        "key_signature": key_signature,
        "genre": infer_genre(float(bpm_int or 100), centroid, rolloff, zcr),
    }


def load_font(size: int) -> ImageFont.FreeTypeFont | ImageFont.ImageFont:
    font_candidates = [
        "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
    ]
    for candidate in font_candidates:
        if os.path.exists(candidate):
            return ImageFont.truetype(candidate, size=size)
    return ImageFont.load_default()


def wrap_text(draw: ImageDraw.ImageDraw, text: str, font: ImageFont.ImageFont, max_width: int) -> list[str]:
    words = text.split()
    lines: list[str] = []
    current = ""
    for word in words:
        test = word if not current else f"{current} {word}"
        if draw.textlength(test, font=font) <= max_width:
            current = test
        else:
            if current:
                lines.append(current)
            current = word
    if current:
        lines.append(current)
    return lines[:2] if lines else [text[:40]]


def build_artwork(base_image: Image.Image, title: str, file_name: str, bpm: str, duration_label: str, out_path: Path) -> None:
    seed = int(hashlib.sha1(file_name.encode("utf-8")).hexdigest()[:8], 16)
    accent = BASE_COLORS[seed % len(BASE_COLORS)]

    # Requested style: hero icon on a black ground with song title.
    merged = Image.new("RGBA", (1200, 1200), (0, 0, 0, 255))
    draw = ImageDraw.Draw(merged)

    hero = ImageOps.contain(base_image.convert("RGBA"), (760, 760), method=Image.Resampling.LANCZOS)
    hero_shadow = Image.new("RGBA", hero.size, (0, 0, 0, 0))
    hero_shadow.paste(hero, (0, 0), hero)
    hero_shadow = hero_shadow.filter(ImageFilter.GaussianBlur(radius=20))
    hx = (1200 - hero.width) // 2
    hy = 120
    merged.alpha_composite(hero_shadow, (hx, hy + 14))
    merged.alpha_composite(hero, (hx, hy))

    draw.rounded_rectangle(
        [(90, 840), (1110, 1130)],
        radius=28,
        fill=(0, 0, 0, 220),
        outline=(accent[0], accent[1], accent[2], 230),
        width=3,
    )

    title_font = load_font(68)
    subtitle_font = load_font(40)
    tiny_font = load_font(28)

    title_lines = wrap_text(draw, title, title_font, max_width=930)
    y = 888
    for line in title_lines:
        line_width = draw.textlength(line, font=title_font)
        draw.text(((1200 - line_width) / 2, y), line, fill=(238, 248, 255, 255), font=title_font)
        y += 78

    producer_text = "prod. North3rnLight3r"
    producer_w = draw.textlength(producer_text, font=subtitle_font)
    draw.text(((1200 - producer_w) / 2, 1044), producer_text, fill=(121, 245, 225, 255), font=subtitle_font)

    info_text = f"BPM {str(bpm or 'N/A')}  |  {duration_label or '00:00'}"
    draw.text((108, 1096), info_text, fill=(188, 218, 240, 255), font=tiny_font)
    draw.text((880, 1096), f"ID {make_key(file_name)[:8].upper()}", fill=(188, 218, 240, 255), font=tiny_font)

    out_path.parent.mkdir(parents=True, exist_ok=True)
    merged.convert("RGB").save(out_path, format="PNG", optimize=True)


def load_existing_map(catalog_path: Path) -> dict[str, dict]:
    if not catalog_path.exists():
        return {}
    try:
        data = json.loads(catalog_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError:
        return {}
    if not isinstance(data, list):
        return {}
    result: dict[str, dict] = {}
    for item in data:
        if isinstance(item, dict) and item.get("file_name"):
            result[str(item["file_name"])] = item
    return result


def make_key(file_name: str) -> str:
    return hashlib.sha1(file_name.encode("utf-8")).hexdigest()[:16]


def website_asset_file_name(file_name: str, size_bytes: int) -> str:
    file_path = Path(file_name)
    if int(size_bytes or 0) > MAX_WEBSITE_ASSET_BYTES:
        return f"{file_path.stem}.preview.mp3"
    return file_path.name


def process_tracks(source_dir: Path, catalog_path: Path, seed_catalog_path: Path, hero_image_path: Path, artwork_dir: Path) -> int:
    if not source_dir.exists():
        raise FileNotFoundError(f"source directory not found: {source_dir}")
    if not hero_image_path.exists():
        raise FileNotFoundError(f"hero image not found: {hero_image_path}")

    files = list_audio_files(source_dir)
    if not files:
        raise RuntimeError(f"no audio files found in: {source_dir}")

    existing_map = load_existing_map(catalog_path)
    base_image = Image.open(hero_image_path).convert("RGB")
    entries = []

    for index, audio_file in enumerate(files, start=1):
        stat = audio_file.stat()
        file_name = audio_file.name
        existing = existing_map.get(file_name, {})
        title = str(existing.get("title") or audio_file.stem).strip() or audio_file.stem

        analysis = analyze_track(audio_file)
        duration_seconds = probe_duration_seconds(audio_file)
        duration_label = format_duration(duration_seconds)
        art_name = f"{slugify(audio_file.stem)}-{make_key(file_name)[:6]}.png"
        art_path = artwork_dir / art_name
        build_artwork(base_image, title, file_name, analysis["bpm"], duration_label, art_path)
        asset_file_name = website_asset_file_name(file_name, int(stat.st_size))
        asset_public_url = f"{WEBSITE_AUDIO_BASE_URL}/{quote(asset_file_name)}"

        entry = {
            "key": str(existing.get("key") or make_key(file_name)),
            "file_name": file_name,
            "asset_file_name": asset_file_name,
            "title": title,
            "description": str(existing.get("description") or "North3rnLight3r Beat"),
            "bpm": analysis["bpm"],
            "key_signature": analysis["key_signature"],
            "tempo": analysis["tempo"],
            "genre": analysis["genre"],
            "price": str(existing.get("price") or "$19.99"),
            "size": int(stat.st_size),
            "uploaded_at": safe_iso_utc(stat.st_mtime),
            "public_url": asset_public_url,
            "artwork_url": f"/assets/artwork/{quote(art_name)}",
            "full_song": True,
            "catalog_label": "Beat Catalog",
            "duration_seconds": round(duration_seconds, 2) if duration_seconds > 0 else 0,
            "duration_label": duration_label,
            "full_song_url": asset_public_url,
        }
        entries.append(entry)
        print(f"[{index:03d}/{len(files):03d}] analyzed {file_name} :: BPM {entry['bpm']} :: {entry['key_signature']} :: {entry['genre']}")

    catalog_path.write_text(json.dumps(entries, indent=2) + "\n", encoding="utf-8")
    seed_catalog_path.parent.mkdir(parents=True, exist_ok=True)
    seed_catalog_path.write_text(json.dumps(entries, indent=2) + "\n", encoding="utf-8")
    return len(entries)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate Beat Catalog metadata and artwork.")
    parser.add_argument(
        "--source-dir",
        default=resolve_source_dir(),
        help="Directory containing source tracks.",
    )
    parser.add_argument(
        "--catalog-path",
        default="server/storage/catalog.json",
        help="Catalog JSON path to update.",
    )
    parser.add_argument(
        "--hero-image",
        default="/home/north3rnlight3r/Pictures/North3rnLight3r_Brand_Assets/North3rnLight3rrMascot.png",
        help="Base hero image used for artwork generation.",
    )
    parser.add_argument(
        "--seed-catalog-path",
        default="apps/website/assets/data/beat_catalog.json",
        help="Tracked website seed catalog JSON path.",
    )
    parser.add_argument(
        "--artwork-dir",
        default="apps/website/assets/artwork",
        help="Output directory for generated artwork files.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    source_dir = Path(resolve_source_dir(args.source_dir)).expanduser().resolve()
    catalog_path = Path(args.catalog_path).expanduser().resolve()
    seed_catalog_path = Path(args.seed_catalog_path).expanduser().resolve()
    hero_image = Path(args.hero_image).expanduser().resolve()
    artwork_dir = Path(args.artwork_dir).expanduser().resolve()

    total = process_tracks(source_dir, catalog_path, seed_catalog_path, hero_image, artwork_dir)
    print(f"Beat Catalog generation complete. tracks={total} artwork_dir={artwork_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
