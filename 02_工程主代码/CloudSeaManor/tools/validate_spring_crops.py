#!/usr/bin/env python3
"""Validate A-9 spring crop expansion requirements."""

from __future__ import annotations

import csv
import sys
from pathlib import Path


REQUIRED_SPRING_IDS = {"turnip", "cloud_berry", "root_turnip", "rutabaga", "bean_pod"}


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    table_path = root / "assets" / "data" / "CropTable.csv"
    if not table_path.exists():
        print(f"[ERROR] missing file: {table_path}")
        return 1

    spring_ids: set[str] = set()
    with table_path.open("r", encoding="utf-8-sig", newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            crop_id = (row.get("id") or "").strip()
            tags = (row.get("tags") or "").strip()
            tag_set = {t.strip() for t in tags.split(";") if t.strip()}
            if "season_spring" in tag_set and crop_id:
                spring_ids.add(crop_id)

    missing = sorted(REQUIRED_SPRING_IDS - spring_ids)
    if missing:
        print(f"[ERROR] missing spring crops: {', '.join(missing)}")
        return 2

    print(f"[OK] spring crop count: {len(spring_ids)}")
    print("[PASS] spring crop validation passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
