#!/usr/bin/env python3
"""Validate dialogue token support coverage."""

from __future__ import annotations

import sys
from pathlib import Path


REQUIRED_TOKENS = ("$[WEATHER]", "$[SEASON]", "$[DAY]", "$[NPC_NAME]", "$[ITEM_NAME]")


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    engine_cpp = root / "src" / "engine" / "DialogueEngine.cpp"
    npc_cpp = root / "src" / "engine" / "NpcDialogueManager.cpp"

    ok = True
    text_engine = engine_cpp.read_text(encoding="utf-8")
    text_npc = npc_cpp.read_text(encoding="utf-8")

    for token in REQUIRED_TOKENS:
        if token not in text_engine and token not in text_npc:
            print(f"[ERROR] token not handled: {token}")
            ok = False
        else:
            print(f"[OK] token handled: {token}")

    if "ctx.item_name" not in text_engine:
        print("[ERROR] DialogueEngine does not use ctx.item_name")
        ok = False
    else:
        print("[OK] DialogueEngine uses ctx.item_name")

    if ok:
        print("[PASS] dialogue token validation passed.")
        return 0
    print("[FAIL] dialogue token validation failed.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
