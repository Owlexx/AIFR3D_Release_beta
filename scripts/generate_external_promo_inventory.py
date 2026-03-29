#!/usr/bin/env python3
"""Generate external AIFR3D promo codes outside the repo.

Writes the actual code inventory to ~/Documents by default so promo values are
never tracked in git.
"""

from __future__ import annotations

import argparse
import json
import os
import secrets
from datetime import datetime, timezone
from pathlib import Path


DEFAULT_OUTPUT = Path.home() / "Documents" / "AIFR3D_promo_codes_2.2.4_beta.json"
DEFAULT_ADMIN_CODE = os.environ.get("AIFR3D_ADMIN_CODE", "Poohbe@r2009$0826").strip()
DEFAULT_LIFETIME_CODE = os.environ.get("AIFR3D_BYPASS_CODE", "N#L09").strip()
FIVE_CHAR_CHARSET = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789#$"
DEFAULT_PRODUCTS = ["vst", "standalone"]


def now_iso() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat()


def random_five_char(existing: set[str]) -> str:
    while True:
        candidate = "".join(secrets.choice(FIVE_CHAR_CHARSET) for _ in range(5))
        if candidate not in existing:
            return candidate


def make_session_bundle(code: str, note: str = "plugin promo") -> dict:
    return {
        "code": code,
        "products": DEFAULT_PRODUCTS,
        "active": True,
        "max_uses": 1,
        "uses": 0,
        "note": note,
        "entitlement_type": "session_bundle",
        "lifetime": False,
        "session_limits": {
            "analyze": 3,
            "compare": 3,
            "reference": 3,
        },
        "created_at": now_iso(),
        "last_redeemed_at": "",
        "payment_bypass": False,
        "admin_code": False,
        "payment_verified": False,
    }


def make_full_bypass(code: str, note: str, *, admin_code: bool = False) -> dict:
    return {
        "code": code,
        "products": DEFAULT_PRODUCTS,
        "active": True,
        "max_uses": 0,
        "uses": 0,
        "note": note,
        "entitlement_type": "lifetime",
        "lifetime": True,
        "session_limits": {
            "analyze": 0,
            "compare": 0,
            "reference": 0,
        },
        "created_at": now_iso(),
        "last_redeemed_at": "",
        "payment_bypass": True,
        "admin_code": admin_code,
        "payment_verified": False,
    }


def build_inventory(total_five_char: int, admin_code: str, lifetime_code: str) -> list[dict]:
    if total_five_char < 1:
        raise ValueError("total_five_char must be at least 1")
    if not admin_code:
        raise ValueError("admin_code is required")
    if not lifetime_code:
        raise ValueError("lifetime_code is required")

    codes: list[dict] = []
    used = {admin_code, lifetime_code}

    codes.append(make_full_bypass(admin_code, "admin install and PayPal bypass", admin_code=True))
    codes.append(make_full_bypass(lifetime_code, "promotional full-access PayPal bypass"))

    while sum(1 for item in codes if item.get("entitlement_type") == "session_bundle") < total_five_char:
        code = random_five_char(used)
        used.add(code)
        codes.append(make_session_bundle(code))

    return codes


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", default=str(DEFAULT_OUTPUT))
    parser.add_argument("--five-char-count", type=int, default=100)
    parser.add_argument("--admin-code", default=DEFAULT_ADMIN_CODE)
    parser.add_argument("--lifetime-code", default=DEFAULT_LIFETIME_CODE)
    args = parser.parse_args()

    output = Path(args.output).expanduser().resolve()
    output.parent.mkdir(parents=True, exist_ok=True)

    inventory = build_inventory(args.five_char_count, args.admin_code, args.lifetime_code)
    output.write_text(json.dumps(inventory, indent=2), encoding="utf-8")

    print(f"Wrote external promo inventory: {output}")
    print(f"Five-char session codes: {args.five_char_count}")
    print("Full-access bypass codes: 2")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
