from __future__ import annotations

import pathlib
import re
import sys


ROOT = pathlib.Path(__file__).resolve().parents[1]
AUDIO_DIR = ROOT / "assets" / "audio"
UI_DIR = ROOT / "assets" / "ui"

AUDIO_REQUIRED = [
    "sfx_ui_confirm_v1.wav",
    "sfx_ui_cancel_v1.wav",
    "sfx_battle_hit_v1.wav",
    "sfx_battle_purify_v1.wav",
    "sfx_mail_arrived_v1.wav",
    "bgm_farm_day_v1.wav",
    "bgm_battle_normal_v1.wav",
]

UI_REQUIRED = [
    "ui_hud_frame_idle_v1.png",
    "ui_mail_panel_idle_v1.png",
    "ui_workshop_panel_idle_v1.png",
    "ui_dialogue_box_idle_v1.png",
    "ui_button_primary_idle_v1.png",
    "ui_button_primary_focus_v1.png",
]

AUDIO_PATTERN = re.compile(r"^(sfx|bgm)_[a-z0-9_]+_v\d+\.(wav|ogg)$")
UI_PATTERN = re.compile(r"^ui_[a-z0-9_]+_v\d+\.png$")


def validate_names(target_dir: pathlib.Path, pattern: re.Pattern[str]) -> list[str]:
    issues: list[str] = []
    if not target_dir.exists():
        issues.append(f"Missing directory: {target_dir}")
        return issues
    for path in target_dir.glob("*"):
        if path.is_file() and not pattern.match(path.name):
            issues.append(f"Invalid name: {path.relative_to(ROOT)}")
    return issues


def validate_required(target_dir: pathlib.Path, required: list[str]) -> list[str]:
    issues: list[str] = []
    for filename in required:
        if not (target_dir / filename).exists():
            issues.append(f"Missing required placeholder: {target_dir.relative_to(ROOT) / filename}")
    return issues


def main() -> int:
    issues: list[str] = []
    issues.extend(validate_names(AUDIO_DIR, AUDIO_PATTERN))
    issues.extend(validate_names(UI_DIR, UI_PATTERN))
    issues.extend(validate_required(AUDIO_DIR, AUDIO_REQUIRED))
    issues.extend(validate_required(UI_DIR, UI_REQUIRED))

    if issues:
        print("Resource spec validation failed:")
        for issue in issues:
            print(f"- {issue}")
        return 1

    print("Resource spec validation passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
