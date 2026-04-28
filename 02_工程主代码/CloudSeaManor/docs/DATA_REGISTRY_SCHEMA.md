# Data Registry Schema

## Purpose
`DataRegistry` is the single runtime registry for externally swappable gameplay data roots.

## Registered Categories
- `crops` -> `assets/data`
- `weapons` -> `assets/data`
- `npcs` -> `assets/data`
- `recipes` -> `assets/data/recipes`
- `fishing` -> `assets/data/fishing`
- `festival` -> `assets/data/festival`
- `diary` -> `assets/data/diary`
- `skills` -> `assets/data/skills`

## Validation Contract
- Startup scans each registered directory.
- Missing directories are logged as `missing|<category>|<path>`.
- Existing directories are logged as `ok|<category>|<path>|files=<n>|hash=<value>`.
- File hashes are aggregate content hashes for quick conflict and override detection.

## Forward Compatibility
- Unknown fields in data files must be ignored by loaders unless the field is required for identity.
- New files can be added under a registered category without changing code paths.
- Mods may override category files by contributing the same relative path through existing data root resolution.

## Intended MVP Use
- Replace one CSV/JSON file under a registered directory.
- Existing examples already covered by runtime resolution include `NPC_Texts.json`, `Gift_Preference.json`, `festival/festival_definitions.csv`, and future split recipe/fishing/skills data.
- Reboot the game.
- Read `DataRegistryCheck` logs to verify the override was discovered and hashed.
