#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import json
import pathlib


def export_csv_to_json(source_csv: pathlib.Path, output_json: pathlib.Path) -> None:
    rows = []
    with source_csv.open("r", encoding="utf-8-sig", newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            rows.append(dict(row))
    output_json.parent.mkdir(parents=True, exist_ok=True)
    output_json.write_text(json.dumps(rows, ensure_ascii=False, indent=2), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Export CSV tables to JSON.")
    parser.add_argument("--input", required=True, help="Input CSV file")
    parser.add_argument("--output", required=True, help="Output JSON file")
    args = parser.parse_args()
    export_csv_to_json(pathlib.Path(args.input), pathlib.Path(args.output))
    print(f"Exported {args.input} -> {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
