#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import pathlib
import re
import sys


ID_SUFFIX_RE = re.compile(r"(Id|Ids|ProfileId|ProfileIds)$")


def validate_csv(path: pathlib.Path) -> list[str]:
    issues: list[str] = []
    with path.open("r", encoding="utf-8-sig", newline="") as f:
        reader = csv.DictReader(f)
        if reader.fieldnames is None:
            return [f"{path}: missing header"]
        headers = reader.fieldnames
        primary_key = "Id" if "Id" in headers else ("NpcId" if "NpcId" in headers else "")
        if not primary_key:
            issues.append(f"{path}: missing required primary key column (Id/NpcId)")

        for h in headers:
            if h and h[0].islower():
                issues.append(f"{path}: header not PascalCase -> {h}")
            if h.endswith("id") and not ID_SUFFIX_RE.search(h):
                issues.append(f"{path}: non-standard id suffix -> {h}")

        seen_ids: set[str] = set()
        for i, row in enumerate(reader, start=2):
            row_id = (row.get(primary_key) or "").strip() if primary_key else ""
            if not row_id:
                issues.append(f"{path}:{i}: empty {primary_key or 'Id'}")
                continue
            unique_key = row_id
            if primary_key == "NpcId" and "Stage" in headers:
                unique_key = f"{row_id}#{(row.get('Stage') or '').strip()}"
            if unique_key in seen_ids:
                issues.append(f"{path}:{i}: duplicate key '{unique_key}'")
            seen_ids.add(unique_key)
            for key, value in row.items():
                if key.endswith("Id") and key != "Id" and value and " " in value:
                    issues.append(f"{path}:{i}: suspicious reference '{key}={value}'")
    return issues


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate CSV table format and references.")
    parser.add_argument("paths", nargs="+", help="CSV files or directories")
    args = parser.parse_args()

    csv_files: list[pathlib.Path] = []
    for p in args.paths:
        path = pathlib.Path(p)
        if path.is_dir():
            csv_files.extend(path.rglob("*.csv"))
        elif path.suffix.lower() == ".csv":
            csv_files.append(path)

    issues: list[str] = []
    for file in csv_files:
        issues.extend(validate_csv(file))

    if not issues:
        print("Validation passed: no issues found.")
        return 0
    print("Validation failed:")
    for issue in issues:
        print(f"- {issue}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
