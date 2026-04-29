# Resource Spec Placeholder Guide

## Audio Placeholder Spec
- Directory: `assets/audio/`
- Naming: `sfx_<system>_<action>_v<number>.wav`
- Format: PCM WAV, 48kHz, 16-bit, mono for SFX; stereo for BGM.
- Loudness target: SFX `-18 LUFS`, BGM `-22 LUFS`, peak below `-1 dBFS`.
- Required placeholders:
  - `sfx_ui_confirm_v1.wav`
  - `sfx_ui_cancel_v1.wav`
  - `sfx_battle_hit_v1.wav`
  - `sfx_battle_purify_v1.wav`
  - `sfx_mail_arrived_v1.wav`
  - `bgm_farm_day_v1.wav`
  - `bgm_battle_normal_v1.wav`

## UI Placeholder Spec
- Directory: `assets/ui/`
- Naming: `ui_<panel>_<element>_<state>_v<number>.png`
- Pixel density: native 1x pixel art; no linear filtering.
- Recommended base palette: 16-color low-saturation palette.
- Panel atlas size: `1024x1024`, RGBA PNG, transparent background.
- Required placeholders:
  - `ui_hud_frame_idle_v1.png`
  - `ui_mail_panel_idle_v1.png`
  - `ui_workshop_panel_idle_v1.png`
  - `ui_dialogue_box_idle_v1.png`
  - `ui_button_primary_idle_v1.png`
  - `ui_button_primary_focus_v1.png`

## Delivery Checklist
- File name follows the naming convention.
- Size and format are valid.
- Asset can be loaded by `ResourceManager`.
- Missing assets must have fallback IDs in config.
