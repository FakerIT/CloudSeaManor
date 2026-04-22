# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2026-04-16

### Added
- Eight-directional player movement
- AABB obstacle collision
- TMX map loading and basic tile rendering
- Interactable object detection and highlighting
- Typed interactions: GatheringNode / Workstation / Storage
- Pickup item generation and auto-collection
- Inventory system prototype
- HUD and inventory text panels
- Stamina consumption and recovery
- Game clock and day progression
- Cloud sea state machine (Clear / Mist / DenseCloud / Tide)
- Debug cloud state toggle (F5)
- Config file loading (gameplay.cfg)
- Logger system
- Crop growth system with cloud density multiplier
- NPC schedule system with CSV/JSON data
- Spirit beast follow/wander behavior
- Skill system with level-up
- Festival system with seasonal events
- Workshop/tea machine processing system
- Cloud Guardian Contract system
- Dynamic Life system for NPCs
- Save/Load system
- Tutorial hints
- Heart particle effects
- Performance frame monitor

### Technical
- C++20 / SFML 3.0.2 / CMake build system
- Four-layer architecture: app / engine / domain / infrastructure
- Coding standards document (CODING_STANDARDS.md)
- Architecture guide (docs/ARCHITECTURE.md)
- Codebase guide with file placement filter (docs/CODEBASE_GUIDE.md)
- .clang-format and .editorconfig configured
