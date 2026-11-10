# Bonus Tiers

Union plugin for **Gothic I** and **Gothic II: Night of the Raven** that separates permanent stat bonuses from teacher **Learning Point (LP)** costs and training limits.

Potions, stone tablets, food, books, prayers, and equipped items no longer make teacher training more expensive or block teacher limits.

## Features

- **Bonus-Independent Training** — Permanent bonuses raise your stats without increasing LP training costs.
- **Point-by-Point Tier Cost** — Crossing a threshold (e.g. `27 -> 32`) calculates each point individually (`3x1 + 2x2 = 7 LP`).
- **Separate Teacher Limits** — Teacher caps check only trained points, not total stats.
- **Character Screen Breakdown (`B`)** — Shows base, permanent bonus, and equipment bonus (`58 +3 +5`, `25/45`, or `Vanilla`).
- **In-Game Menu** — Configurable live under `Options -> Union & Plugins -> Bonus Tiers` (English, Polish, German, Czech), including an instant on/off plugin toggle.
- **Savegame Compatible** — Works on new games and migrates existing saves automatically.

## Installation

1. Make sure **Union 1.0m+** is installed.
2. Download **`Bonus_Tiers.vdf`** from [**Releases**](../../releases).
3. Copy `Bonus_Tiers.vdf` to `Data\Plugins\` (or `Data\`).

## Configuration

Settings are available in the in-game **Union & Plugins** menu or in `System\Gothic.ini` (`[BONUS_TIERS]`) and `System\Bonus_Tiers.ini`:

```ini
MaxTeachingThreshold = True
MaxFightingSkillThreshold = True
```
