#!/usr/bin/env python3
import csv
import os
import sys

VALID_SEASONS = {"spring", "summer", "autumn", "winter"}


def main() -> int:
    root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    csv_path = os.path.join(root, "assets", "data", "CropTable.csv")

    if not os.path.exists(csv_path):
        print(f"ERROR: file not found: {csv_path}")
        return 1

    errors = []
    warnings = []
    with open(csv_path, "r", encoding="utf-8-sig", newline="") as f:
        reader = csv.DictReader(f)
        for line_no, row in enumerate(reader, start=2):
            def get_int(name: str):
                raw = (row.get(name) or "").strip()
                if not raw:
                    errors.append(f"[ERROR] line {line_no}: {name} 字段无效: <empty>")
                    return None
                try:
                    return int(raw)
                except ValueError:
                    errors.append(f"[ERROR] line {line_no}: {name} 字段无效: {raw}")
                    return None

            growth_time = get_int("growth_time")
            stages = get_int("stages")
            base_harvest = get_int("base_harvest")

            if growth_time is not None and growth_time <= 0:
                errors.append(f"[ERROR] line {line_no}: growth_time 字段无效: {growth_time}")
            if stages is not None and stages < 1:
                errors.append(f"[ERROR] line {line_no}: stages 字段无效: {stages}")
            if base_harvest is not None and base_harvest < 1:
                errors.append(f"[ERROR] line {line_no}: base_harvest 字段无效: {base_harvest}")

            seed_item_id = (row.get("seed_item_id") or "").strip()
            harvest_item_id = (row.get("harvest_item_id") or "").strip()
            if not seed_item_id:
                errors.append(f"[ERROR] line {line_no}: seed_item_id 字段无效: <empty>")
            if not harvest_item_id:
                errors.append(f"[ERROR] line {line_no}: harvest_item_id 字段无效: <empty>")

            season = (row.get("season") or "").strip().lower()
            if season:
                if season not in VALID_SEASONS:
                    errors.append(
                        f"[ERROR] line {line_no}: season 字段无效: {season}"
                    )
            else:
                warnings.append(f"[WARN] line {line_no}: season 字段缺失")

    if errors:
        for item in errors:
            print(item)
        for item in warnings:
            print(item)
        return 2

    for item in warnings:
        print(item)
    print("All checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
