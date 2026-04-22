#!/usr/bin/env python3
"""
Validate daily dialogue quality gates.

Rules:
1) Each core NPC file contains >= 20 entries.
2) No repeated `$[PLAYER_NAME]` token sequence like `$[PLAYER_NAME]$[PLAYER_NAME]`.
"""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path


CORE_NPCS = ("acha", "lin", "xiaoman", "wanxing")
REPEAT_TOKEN = re.compile(r"(\$\[PLAYER_NAME\]){2,}")


def count_entries(payload: dict) -> int:
    total = 0
    for key in ("greetings", "small_talks", "farewells"):
        items = payload.get(key, [])
        if isinstance(items, list):
            total += len(items)
    return total


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    data_dir = root / "assets" / "data" / "daily_dialogue"
    ok = True

    for npc in CORE_NPCS:
        file_path = data_dir / f"npc_daily_{npc}.json"
        if not file_path.exists():
            print(f"[ERROR] missing file: {file_path}")
            ok = False
            continue

        try:
            payload = json.loads(file_path.read_text(encoding="utf-8"))
        except Exception as exc:
            print(f"[ERROR] json parse failed: {file_path} ({exc})")
            ok = False
            continue

        total = count_entries(payload)
        if total < 20:
            print(f"[ERROR] {npc} entries too few: {total} < 20")
            ok = False
        else:
            print(f"[OK] {npc} entries: {total}")

        for section in ("greetings", "small_talks", "farewells"):
            for idx, item in enumerate(payload.get(section, [])):
                text = str(item.get("text", ""))
                if REPEAT_TOKEN.search(text):
                    print(f"[ERROR] {npc}:{section}[{idx}] repeated $[PLAYER_NAME] token")
                    ok = False

    if ok:
        print("[PASS] daily dialogue validation passed.")
        return 0
    print("[FAIL] daily dialogue validation failed.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
