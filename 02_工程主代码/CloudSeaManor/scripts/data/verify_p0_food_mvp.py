#!/usr/bin/env python3
from __future__ import annotations

import csv
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
RECIPES = ROOT / "assets" / "data" / "recipes.csv"
HUNGER = ROOT / "assets" / "data" / "HungerTable.csv"
BUFFS = ROOT / "assets" / "data" / "food" / "food_buff_profiles.csv"


def read_rows(path: Path) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8-sig", newline="") as f:
        return list(csv.DictReader(f))


def main() -> int:
    recipe_rows = read_rows(RECIPES)
    hunger_rows = read_rows(HUNGER)
    buff_rows = read_rows(BUFFS)

    required_recipes = {"cook_tea_egg", "cook_veggie_soup", "cook_warm_milk"}
    outputs = {}
    for row in recipe_rows:
        rid = row.get("id", "").strip()
        if rid in required_recipes:
            outputs[rid] = row.get("output_item", "").strip()

    if set(outputs.keys()) != required_recipes:
        missing = sorted(required_recipes - set(outputs.keys()))
        raise SystemExit(f"Missing recipes: {missing}")

    hunger_ids = {row.get("item_id", "").strip() for row in hunger_rows}
    buff_ids = {row.get("ItemId", "").strip() for row in buff_rows}
    for recipe_id, output in outputs.items():
        if output not in hunger_ids:
            raise SystemExit(f"{recipe_id} output '{output}' not found in HungerTable.csv")
        if output not in buff_ids:
            raise SystemExit(f"{recipe_id} output '{output}' not found in food_buff_profiles.csv")

    print("P0-002 food MVP verification passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
