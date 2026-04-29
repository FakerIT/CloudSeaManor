#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import re
import sys


ROOT = pathlib.Path(__file__).resolve().parents[1]
SRC = ROOT / "src"
INCLUDE = ROOT / "include" / "CloudSeamanor"
PATTERN = re.compile(r'^\s*#\s*include\s*[<"]([^">]+)[">]')


def iter_files() -> list[pathlib.Path]:
    files: list[pathlib.Path] = []
    for base in (SRC, INCLUDE):
        if not base.exists():
            continue
        for path in base.rglob("*"):
            if path.suffix.lower() in {".hpp", ".cpp"}:
                files.append(path)
    return files


def classify(path: pathlib.Path) -> str:
    rel = path.relative_to(ROOT).as_posix()
    if "/domain/" in rel:
        return "domain"
    if "/infrastructure/" in rel:
        return "infrastructure"
    if "/engine/" in rel:
        return "engine"
    if "/app/" in rel:
        return "app"
    return "other"


def main() -> int:
    violations: list[str] = []
    for path in iter_files():
        layer = classify(path)
        for idx, line in enumerate(path.read_text(encoding="utf-8", errors="ignore").splitlines(), start=1):
            m = PATTERN.match(line)
            if not m:
                continue
            target = m.group(1)
            lower = target.lower()
            if layer == "domain":
                if "sfml/" in lower or "cloudseamanor/engine/" in lower or "cloudseamanor/infrastructure/" in lower:
                    violations.append(f"{path.relative_to(ROOT)}:{idx} domain illegal include -> {target}")
            if layer == "infrastructure":
                if "cloudseamanor/engine/" in lower:
                    violations.append(f"{path.relative_to(ROOT)}:{idx} infrastructure illegal include -> {target}")

    print("=== Layer Rule Scan Report ===")
    print(f"Scanned files: {len(iter_files())}")
    if not violations:
        print("No layer violations detected.")
        return 0
    print(f"Violations: {len(violations)}")
    for item in violations:
        print(item)
    return 1


if __name__ == "__main__":
    sys.exit(main())
