#!/usr/bin/env python3

from __future__ import annotations

import math
import os
import subprocess
import sys
from pathlib import Path
from xml.sax.saxutils import escape


WIDTH = 1920
HEIGHT = 1080

BG = "#080A0E"
PANEL = "#10141C"
PANEL_ALT = "#0C1017"
TEXT = "#EEF2FA"
TEXT2 = "#B0BACE"
TEAL = "#00C6BA"
CYAN = "#1C7AD6"
GOLD = "#DCA83E"
PURPLE = "#7A5CE4"
RED = "#E05252"
GREEN = "#39FF88"
STROKE = "#6F86A855"


def panel(x, y, w, h, title, subtitle=None):
    bits = [
        f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="18" fill="{PANEL}" stroke="{STROKE}" stroke-width="1.2"/>',
        text_block(x + 18, y + 30, title, 24, TEXT, weight="700"),
    ]
    if subtitle:
        bits.append(text_block(x + 18, y + 56, subtitle, 13, TEXT2))
    return "\n".join(bits)


def text_block(x, y, text, size=16, color=TEXT, weight="400", line_gap=1.35):
    lines = text.split("\n")
    out = [
        f'<text x="{x}" y="{y}" fill="{color}" font-family="Arial, Helvetica, sans-serif" font-size="{size}" font-weight="{weight}">'
    ]
    for i, line in enumerate(lines):
        dy = 0 if i == 0 else size * line_gap
        out.append(f'<tspan x="{x}" dy="{dy}">{escape(line)}</tspan>')
    out.append("</text>")
    return "\n".join(out)


def badge(cx, cy, label, color):
    return "\n".join(
        [
            f'<circle cx="{cx}" cy="{cy}" r="18" fill="{color}" stroke="{TEXT}" stroke-width="1.4"/>',
            f'<text x="{cx}" y="{cy + 6}" text-anchor="middle" fill="{TEXT}" font-family="Arial, Helvetica, sans-serif" font-size="18" font-weight="700">{escape(label)}</text>',
        ]
    )


def button(x, y, w, h, label, active=False, accent=CYAN):
    fill = accent if active else PANEL_ALT
    stroke = accent if active else STROKE
    text_color = TEXT if active else TEXT2
    return "\n".join(
        [
            f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="12" fill="{fill}" stroke="{stroke}" stroke-width="1.2"/>',
            f'<text x="{x + w/2}" y="{y + h/2 + 6}" text-anchor="middle" fill="{text_color}" font-family="Arial, Helvetica, sans-serif" font-size="16" font-weight="700">{escape(label)}</text>',
        ]
    )


def wrap_text(text, width):
    words = text.split()
    lines = []
    current = ""
    for word in words:
        test = word if not current else current + " " + word
        if len(test) <= width:
            current = test
        else:
            if current:
                lines.append(current)
            current = word
    if current:
        lines.append(current)
    return "\n".join(lines)


def fingerprint_shape(cx, cy, radius, color, fill_alpha="0.15", stroke_alpha="0.90"):
    axes = [0.72, 0.63, 0.66, 0.58, 0.62, 0.69, 0.57, 0.54, 0.61, 0.71, 0.64, 0.68]
    pts = []
    for i, axis in enumerate(axes):
        angle = -math.pi / 2 + (2 * math.pi * i / len(axes))
        px = cx + math.cos(angle) * radius * axis
        py = cy + math.sin(angle) * radius * axis
        pts.append(f"{px:.1f},{py:.1f}")
    poly = " ".join(pts)
    return "\n".join(
        [
            f'<polygon points="{poly}" fill="{color}" fill-opacity="{fill_alpha}" stroke="{color}" stroke-opacity="{stroke_alpha}" stroke-width="3"/>'
        ]
    )


def halo_graphic(x, y, w, h, accent, mode_label, compare=False, reference=False):
    cx = x + w * 0.5
    cy = y + h * 0.53
    r = min(w, h) * 0.28
    inner = r - 18
    if compare:
        labels = ["TONE D", "WIDTH D", "PEAK D", "PUNCH D"]
    elif reference:
        labels = ["TONE", "STEREO", "LOUD", "DYNAMICS"]
    else:
        labels = ["TONE", "WIDTH", "PEAK", "PUNCH"]

    bits = [
        f'<circle cx="{cx}" cy="{cy}" r="{r+22}" fill="none" stroke="{accent}" stroke-opacity="0.15" stroke-width="8"/>',
        f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="none" stroke="{TEXT2}" stroke-opacity="0.15" stroke-width="16"/>',
    ]
    for i, lab in enumerate(labels):
        start = -90 + i * 90 + 8
        end = start + 62
        bits.append(
            describe_arc(cx, cy, r, start, end, [CYAN, GOLD, GREEN, RED][i], 16)
        )
        mid = math.radians((start + end) / 2)
        lx = cx + math.cos(mid) * (r + 36)
        ly = cy + math.sin(mid) * (r + 36)
        bits.append(text_block(lx - 30, ly + 5, lab, 12, TEXT2, weight="700"))

    bits.append(f'<circle cx="{cx}" cy="{cy}" r="{inner}" fill="{PANEL_ALT}" stroke="{accent}" stroke-opacity="0.25" stroke-width="2"/>')
    bits.append(f'<path d="M {x+42} {cy+38} C {x+95} {cy-18}, {x+w-85} {cy+24}, {x+w-42} {cy-12}" stroke="{PURPLE if reference else CYAN}" stroke-width="3" fill="none" stroke-opacity="0.85"/>')
    bits.append(text_block(cx - 72, cy - 6, mode_label, 18, TEXT, weight="700"))
    bits.append(text_block(cx - 56, cy + 24, "center readout", 12, TEXT2))
    return "\n".join(bits)


def describe_arc(cx, cy, r, start_deg, end_deg, color, stroke_width):
    start = polar_to_cartesian(cx, cy, r, end_deg)
    end = polar_to_cartesian(cx, cy, r, start_deg)
    large = 1 if end_deg - start_deg > 180 else 0
    path = (
        f"M {start[0]:.2f} {start[1]:.2f} "
        f"A {r:.2f} {r:.2f} 0 {large} 0 {end[0]:.2f} {end[1]:.2f}"
    )
    return f'<path d="{path}" fill="none" stroke="{color}" stroke-width="{stroke_width}" stroke-linecap="round"/>'


def polar_to_cartesian(cx, cy, radius, angle_deg):
    angle = math.radians(angle_deg - 90)
    return (cx + radius * math.cos(angle), cy + radius * math.sin(angle))


def stereo_graphic(x, y, w, h, accent, mode="live"):
    cx = x + w * 0.5
    cy = y + h * 0.55
    bits = [
        f'<ellipse cx="{cx}" cy="{cy}" rx="{w*0.28}" ry="{h*0.18}" fill="none" stroke="{TEXT2}" stroke-opacity="0.20" stroke-width="2"/>',
        f'<line x1="{cx}" y1="{y+68}" x2="{cx}" y2="{y+h-28}" stroke="{TEXT2}" stroke-opacity="0.15"/>',
        f'<line x1="{x+24}" y1="{cy}" x2="{x+w-24}" y2="{cy}" stroke="{TEXT2}" stroke-opacity="0.15"/>',
        f'<polygon points="{cx},{cy-h*0.16} {cx+w*0.22},{cy} {cx},{cy+h*0.18} {cx-w*0.18},{cy}" fill="{accent}" fill-opacity="0.16" stroke="{accent}" stroke-width="3"/>',
        text_block(x + 20, y + h - 68, "W = mid  X = left  Y = right  Z = side", 13, TEXT2),
        text_block(x + 20, y + h - 42, "state: HEALTHY STEREO" if mode == "live" else ("state: A/B FIELD DELTA" if mode == "compare" else "state: LIVE VS TARGET FIELD"), 14, TEXT, weight="700"),
    ]
    if mode == "compare":
        bits.append(f'<polygon points="{cx},{cy-h*0.12} {cx+w*0.26},{cy} {cx},{cy+h*0.14} {cx-w*0.16},{cy}" fill="{PURPLE}" fill-opacity="0.08" stroke="{PURPLE}" stroke-width="2"/>')
    if mode == "reference":
        bits.append(f'<ellipse cx="{cx}" cy="{cy}" rx="{w*0.23}" ry="{h*0.14}" fill="none" stroke="{PURPLE}" stroke-opacity="0.55" stroke-width="2"/>')
    return "\n".join(bits)


def candle_graphic(x, y, w, h, compare=False, reference=False):
    live_x = x + 18
    live_w = w * 0.26
    hist_x = x + live_w + 34
    hist_w = w - (hist_x - x) - 18
    bits = [
        f'<rect x="{live_x}" y="{y+58}" width="{live_w}" height="{h-86}" rx="10" fill="{PANEL_ALT}" stroke="{STROKE}" stroke-width="1"/>',
        f'<rect x="{hist_x}" y="{y+58}" width="{hist_w}" height="{h-86}" rx="10" fill="{PANEL_ALT}" stroke="{STROKE}" stroke-width="1"/>',
        text_block(live_x + 14, y + 48, "REALTIME BUFFER CANDLES", 12, TEXT2, weight="700"),
        text_block(hist_x + 14, y + 48, "LAST 10 FINISHED SESSIONS" if not compare else "LAST 10 DELTAS", 12, TEXT2, weight="700"),
    ]
    for i in range(5):
        cx = live_x + 26 + i * 24
        bits.append(f'<line x1="{cx}" y1="{y+90}" x2="{cx}" y2="{y+h-52}" stroke="{TEXT2}" stroke-opacity="0.28" stroke-width="1.2"/>')
        body_top = y + 110 + (i % 2) * 14
        body_h = 40 + (i % 3) * 10
        bits.append(f'<rect x="{cx-7}" y="{body_top}" width="14" height="{body_h}" rx="4" fill="{GREEN if i % 2 == 0 else GOLD}" fill-opacity="0.78"/>')
    for i in range(10):
        cx = hist_x + 24 + i * ((hist_w - 48) / 10)
        bits.append(f'<line x1="{cx}" y1="{y+88}" x2="{cx}" y2="{y+h-54}" stroke="{TEXT2}" stroke-opacity="0.20" stroke-width="1"/>')
        top = y + 108 + ((i * 9) % 36)
        bh = 34 + ((i * 7) % 30)
        fill = GREEN if i % 3 == 0 else (RED if i % 3 == 1 else GOLD)
        bits.append(f'<rect x="{cx-6}" y="{top}" width="12" height="{bh}" rx="4" fill="{fill}" fill-opacity="0.74"/>')
    footer = "Wicks = loudness range | Body = window drift"
    if compare:
        footer = "Realtime candles stay live | history text shifts toward delta reading"
    elif reference:
        footer = "Reference mode keeps live candles but reads them against target context"
    bits.append(text_block(x + 18, y + h - 20, footer, 13, TEXT2))
    return "\n".join(bits)


def fixlist_graphic(x, y, w, h):
    bits = []
    for i, (title, why) in enumerate(
        [
            ("Low-mid masking clarity", "Why: dense 250-500 Hz energy"),
            ("Stereo image pulling inward", "Next: reopen width safely"),
        ]
    ):
        cy = y + 50 + i * 76
        bits.append(f'<rect x="{x+16}" y="{cy}" width="{w-32}" height="62" rx="12" fill="{PANEL_ALT}" stroke="{STROKE}" stroke-width="1"/>')
        bits.append(text_block(x + 30, cy + 22, title, 15, TEXT, weight="700"))
        bits.append(text_block(x + 30, cy + 44, why, 12, TEXT2))
    return "\n".join(bits)


def chat_graphic(x, y, w, h, include_controls=True):
    bits = [
        f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="18" fill="{PANEL}" stroke="{STROKE}" stroke-width="1.2"/>',
        text_block(x + 18, y + 26, "INSIGHT CHAT", 18, TEXT, weight="700"),
        f'<rect x="{x+18}" y="{y+44}" width="{w-36}" height="{h-104}" rx="10" fill="{PANEL_ALT}" stroke="{STROKE}" stroke-width="1"/>',
        text_block(x + 30, y + 72, "AIFR3D: Live metrics are driving the current\nreadout. Review the main issue first, then\nconfirm it in the supporting meters.", 13, TEXT2),
        f'<rect x="{x+18}" y="{y+h-46}" width="{w-120}" height="28" rx="8" fill="{PANEL_ALT}" stroke="{STROKE}" stroke-width="1"/>',
        button(x + w - 92, y + h - 46, 74, 28, "Ask AI", active=True, accent=TEAL),
    ]
    if include_controls:
        bits.append(button(x + 18, y + h - 82, 118, 28, "Pulse: Loudness", active=False, accent=CYAN))
        bits.append(button(x + 144, y + h - 82, 96, 28, "Clear Fixes", active=False, accent=CYAN))
        bits.append(button(x + 248, y + h - 82, 96, 28, "Attach File", active=False, accent=CYAN))
    return "\n".join(bits)


def legend_column(mode_name, accent, heading, bullets, cues):
    x = 1300
    y = 50
    w = 580
    h = 980
    parts = [
        f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="22" fill="{PANEL}" stroke="{STROKE}" stroke-width="1.4"/>',
        text_block(x + 28, y + 38, f"{mode_name} MODE MAP", 30, accent, weight="700"),
        text_block(x + 28, y + 64, "Code-derived schematic at 1920x1080. This is based on the current UI code and default layout A, not a live runtime capture.", 14, TEXT2),
        text_block(x + 28, y + 116, "HOW TO READ THIS MODE", 20, TEXT, weight="700"),
        text_block(x + 28, y + 146, heading, 16, TEXT2),
    ]

    cursor_y = y + 194
    for idx, bullet in enumerate(bullets, start=1):
        parts.append(badge(x + 42, cursor_y - 6, str(idx), accent))
        parts.append(text_block(x + 72, cursor_y, wrap_text(bullet, 42), 16, TEXT))
        cursor_y += 94

    parts.append(text_block(x + 28, y + 732, "MODE CUES", 20, TEXT, weight="700"))
    parts.append(text_block(x + 28, y + 764, cues, 16, TEXT2))
    parts.append(text_block(x + 28, y + 904, "Current quick controls in code: Analyze / Compare / Reference, layout A-D, pulse mode, compare snapshot buttons, reference pinning, fix-list tab, mix-tips tab, API key controls, debug toggle.", 15, TEXT))
    return "\n".join(parts)


def topbar(mode_name, accent):
    x = 60
    y = 56
    w = 1180
    h = 92
    return "\n".join(
        [
            f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="20" fill="{PANEL}" stroke="{STROKE}" stroke-width="1.2"/>',
            f'<circle cx="{x+38}" cy="{y+46}" r="22" fill="{PANEL_ALT}" stroke="{accent}" stroke-width="2"/>',
            text_block(x + 72, y + 36, "AIFR3D VST A", 32, TEAL, weight="700"),
            text_block(x + 72, y + 62, "Version 2.2.4 | status line | detected genre confidence", 14, TEXT2),
            button(x + 710, y + 22, 96, 34, "Analyze", active=mode_name == "ANALYZE", accent=CYAN),
            button(x + 814, y + 22, 96, 34, "Reference", active=mode_name == "REFERENCE", accent=PURPLE),
            button(x + 918, y + 22, 96, 34, "Compare", active=mode_name == "COMPARE", accent=GOLD),
            text_block(x + 1028, y + 34, "Theme", 14, TEXT2),
            button(x + 1080, y + 22, 54, 34, "A", active=True, accent=accent),
            f'<rect x="{x+958}" y="{y+58}" width="156" height="24" rx="12" fill="{accent}" fill-opacity="0.14" stroke="{accent}" stroke-opacity="0.55"/>',
            text_block(x + 1005, y + 76, mode_name, 14, TEXT, weight="700"),
        ]
    )


def make_svg(mode_name, mode_kind):
    accent = {"analyze": CYAN, "compare": GOLD, "reference": PURPLE}[mode_kind]
    title = {
        "analyze": "MIX SIGNATURE (LIVE SESSION)",
        "compare": "MIX SIGNATURE (A/B DELTA)",
        "reference": "MIX SIGNATURE (REFERENCE ALIGNMENT)",
    }[mode_kind]
    title2 = {
        "analyze": "LIVE DIAGNOSTIC HALO",
        "compare": "A/B DELTA HALO",
        "reference": "REFERENCE ALIGNMENT HALO",
    }[mode_kind]
    title3 = {
        "analyze": "SPATIAL FIELD (LIVE)",
        "compare": "SPATIAL FIELD (COMPARE)",
        "reference": "SPATIAL FIELD (REFERENCE)",
    }[mode_kind]
    title4 = {
        "analyze": "LIVE SESSION CANDLE + LAST 10",
        "compare": "LIVE CANDLE + LAST 10 DELTAS",
        "reference": "REFERENCE SESSION CANDLES",
    }[mode_kind]
    mode_heading = {
        "analyze": "Read cyan live instruments first. Analyze mode is the only mode where the live mix is the main story and reference context should not dominate the visual language.",
        "compare": "Read signed differences first. Compare mode uses captured Mix A, captured Mix B, or Mix A versus live B and shifts the main panels into delta-reading behavior.",
        "reference": "Read live value plus target context. Reference mode keeps the live measurement active but adds purple target overlays, corridor colors, and target-oriented helper text.",
    }[mode_kind]
    bullets = {
        "analyze": [
            "Top bar. The active mode pill is Analyze, the current layout is A by default, and the top row remains the fastest place to switch modes and layouts.",
            "Mix Signature panel. This is the main fingerprint and metric-card surface. In Analyze mode it shows the live mix only: fingerprint, loudness card, true peak card, width card, punch card, and band-energy rows.",
            "Diagnostic halo. Four live segments summarize tone, width, peak, and punch. The outer pulse follows the selected pulse focus. The inner line is the live spectrum layer.",
            "Spatial Field. Read W as mid, X as left, Y as right, Z as side. The polygon and state line tell you whether the stereo field is healthy, too narrow, or phase-risky.",
            "Candle Storyline. Left side is rolling realtime buffer candles. Right side stores the last ten finished sessions. Wicks show range; bodies show drift.",
            "Fix List and Insight Chat. The fix list turns the current dominant problem into plain-English actions. The chat panel preserves the current advisory session and quick controls.",
        ],
        "compare": [
            "Top bar. The active mode pill changes to Compare. The current code keeps the same overall layout but switches panel titles and meanings to A versus B or A versus live B.",
            "Mix Signature panel. Fingerprints overlay Mix A and Mix B. The key question becomes: what changed between the two sources, not whether the live mix fits a target corridor.",
            "A/B Delta Halo. The four segments become TONE D, WIDTH D, PEAK D, and PUNCH D. Segment size shows magnitude of difference; color tells whether Mix B is lower, similar, or hotter/wider than Mix A.",
            "Spatial Field Compare. Purple represents Mix A and cyan represents Mix B or live B. This panel helps you see left/right/center/side differences without reference-pool logic.",
            "Compare candles. The candle panel keeps a live side and a history side, but the mode title and helper text shift toward delta reading instead of pure live-session coaching.",
            "Compare menu. This mode uniquely exposes Capture Mix A, Capture Mix B, and Use Live B. Those controls appear above the fix-list area in the current code.",
        ],
        "reference": [
            "Top bar. The active mode pill changes to Reference. The layout remains the same by default, but the mode-specific menu area appears and the panel titles shift to target-alignment language.",
            "Mix Signature panel. Cyan stays the live fingerprint. Purple becomes the target ghost or corridor overlay. This panel answers: how does the current mix differ from the canonical target?",
            "Reference Alignment Halo. Arc length still comes from live measurement. Color changes now reflect corridor evaluation rather than pure live-state color logic. Purple is the target accent, not the live value.",
            "Spatial Field Reference. Cyan is the current field. Purple is the reference width/correlation target. Read this as live-versus-target stereo behavior, not as an A/B capture view.",
            "Reference candles. The candle panel keeps session-memory behavior but the title and interpretation shift toward reference-session reading rather than pure live-only storytelling.",
            "Reference menu. This mode uniquely exposes Reference Genre and Pin Reference. If target data is missing, the code falls back to neutral live measurement mode rather than faking a target.",
        ],
    }[mode_kind]
    cues = {
        "analyze": "Look for cyan-led live values, no mode menu block above the fix-list area, and panel titles that say LIVE. This is the fastest diagnostic cockpit.",
        "compare": "Look for the compare control block above the tabs, A/B wording in titles, and delta language in the halo and fingerprint explanations.",
        "reference": "Look for the reference control block above the tabs, purple target accents, and helper text describing live-versus-target corridor behavior.",
    }[mode_kind]

    svg = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{WIDTH}" height="{HEIGHT}" viewBox="0 0 {WIDTH} {HEIGHT}">',
        f'<rect width="{WIDTH}" height="{HEIGHT}" fill="{BG}"/>',
        topbar(mode_name, accent),
        panel(60, 168, 684, 514, title, "12-axis fingerprint, metric cards, and band rows"),
        fingerprint_shape(332, 418, 148, CYAN if mode_kind != "compare" else GOLD, fill_alpha="0.12", stroke_alpha="0.95"),
        *[describe_arc(332, 418, 176, -90 + i * 30, -72 + i * 30, TEXT2, 1) for i in range(12)],
        text_block(110, 592, "cards: Loudness | True Peak | Width | Punch\nbands: Sub Low Low-Mid Mid High-Mid High Air", 16, TEXT2),
        panel(60, 694, 684, 226, title4, "Realtime buffer candles on the left, last ten sessions on the right"),
        candle_graphic(60, 694, 684, 226, compare=(mode_kind == "compare"), reference=(mode_kind == "reference")),
        panel(752, 168, 282, 392, title2, "Four-segment meter with pulse ring and inner spectrum"),
        halo_graphic(752, 168, 282, 392, accent, mode_name if mode_kind != "compare" else "A/B DELTA", compare=(mode_kind == "compare"), reference=(mode_kind == "reference")),
        panel(1042, 168, 198, 392, title3, "Measured WXYZ field plus stereo state"),
        stereo_graphic(1042, 168, 198, 392, CYAN, mode="live" if mode_kind == "analyze" else mode_kind),
    ]

    rb_y = 570
    rb_h = 350
    if mode_kind in {"compare", "reference"}:
        panel_title = "COMPARE MENU" if mode_kind == "compare" else "REFERENCE MENU"
        panel_sub = "Capture Mix A / Mix B / Live B" if mode_kind == "compare" else "Reference genre and pin controls"
        svg.extend(
            [
                panel(752, rb_y, 488, 120, panel_title, panel_sub),
                button(772, rb_y + 54, 120, 30, "Mix A" if mode_kind == "compare" else "Genre", active=False, accent=accent),
                button(900, rb_y + 54, 112, 30, "Mix B" if mode_kind == "compare" else "Pin Ref", active=False, accent=accent),
                button(1020, rb_y + 54, 112, 30, "Live B" if mode_kind == "compare" else "Live Only", active=False, accent=accent),
                button(752, 700, 94, 28, "Fix List", active=True, accent=CYAN),
                button(852, 700, 94, 28, "Mix Tips", active=False, accent=CYAN),
                panel(752, 734, 488, 170, "FIX LIST", "Plain-English current issue cards"),
                fixlist_graphic(752, 734, 488, 170),
                chat_graphic(752, 910, 488, 110, include_controls=False),
            ]
        )
    else:
        svg.extend(
            [
                button(752, rb_y, 94, 28, "Fix List", active=True, accent=CYAN),
                button(852, rb_y, 94, 28, "Mix Tips", active=False, accent=CYAN),
                panel(752, 604, 488, 178, "FIX LIST", "Plain-English current issue cards"),
                fixlist_graphic(752, 604, 488, 178),
                chat_graphic(752, 790, 488, 130, include_controls=True),
            ]
        )

    svg.extend(
        [
            badge(82, 190, "1", accent),
            badge(82, 716, "5", accent),
            badge(774, 190, "3", accent),
            badge(1064, 190, "4", accent),
            badge(774, 606 if mode_kind == "analyze" else 756, "6", accent),
            badge(82, 212, "2", accent),
            legend_column(mode_name, accent, mode_heading, bullets, cues),
            "</svg>",
        ]
    )
    return "\n".join(svg)


def write_mode(outdir: Path, stem: str, mode_name: str, mode_kind: str) -> None:
    svg_path = outdir / f"{stem}.svg"
    png_path = outdir / f"{stem}.png"
    svg_path.write_text(make_svg(mode_name, mode_kind), encoding="utf-8")
    subprocess.run(
        ["magick", str(svg_path), "-background", "none", "-density", "144", "-resize", f"{WIDTH}x{HEIGHT}", str(png_path)],
        check=True,
    )


def main(argv: list[str]) -> int:
    outdir = Path(argv[1]) if len(argv) > 1 else Path("docs/generated/mode_maps")
    outdir.mkdir(parents=True, exist_ok=True)
    write_mode(outdir, "AIFR3D_MODE_MAP_ANALYZE_1920x1080", "ANALYZE", "analyze")
    write_mode(outdir, "AIFR3D_MODE_MAP_COMPARE_1920x1080", "COMPARE", "compare")
    write_mode(outdir, "AIFR3D_MODE_MAP_REFERENCE_1920x1080", "REFERENCE", "reference")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
