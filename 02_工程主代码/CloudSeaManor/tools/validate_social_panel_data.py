#!/usr/bin/env python3
"""Validate social panel backing data for core NPCs."""

from __future__ import annotations

import sys
from pathlib import Path


CORE_NPCS = ("acha", "lin", "xiaoman", "wanxing")


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    daily_dir = root / "assets" / "data" / "daily_dialogue"
    heart_dir = root / "assets" / "data" / "dialogue"

    ok = True
    for npc in CORE_NPCS:
        daily = daily_dir / f"npc_daily_{npc}.json"
        if not daily.exists():
            print(f"[ERROR] missing daily dialogue: {daily.name}")
            ok = False
        else:
            print(f"[OK] {daily.name}")

        for heart in (1, 2, 3):
            heart_file = heart_dir / f"npc_heart_{npc}_h{heart}.json"
            if not heart_file.exists():
                print(f"[ERROR] missing heart event: {heart_file.name}")
                ok = False
            else:
                print(f"[OK] {heart_file.name}")

    if ok:
        print("[PASS] social panel data validation passed.")
        return 0
    print("[FAIL] social panel data validation failed.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
