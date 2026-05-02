#!/usr/bin/env python3
"""Validate h3 heart-event assets for four core NPCs."""

from __future__ import annotations

import sys
from pathlib import Path


CORE_NPCS = ("acha", "xiaoman", "wanxing", "lin")


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    dialogue_dir = root / "assets" / "data" / "dialogue"
    ok = True

    for npc in CORE_NPCS:
        path = dialogue_dir / f"npc_heart_{npc}_h3.json"
        if not path.exists():
            print(f"[ERROR] missing: {path}")
            ok = False
            continue
        text = path.read_text(encoding="utf-8").strip()
        if "\"nodes\"" not in text or "\"id\"" not in text:
            print(f"[ERROR] invalid h3 payload: {path}")
            ok = False
        else:
            print(f"[OK] {path.name}")

    if ok:
        print("[PASS] h3 assets validation passed.")
        return 0
    print("[FAIL] h3 assets validation failed.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
