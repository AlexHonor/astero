# Base Mode

## Overview

Between missions the player manages their space station base. This is a menu-driven UI (no movement — purely click-based panels). The player processes ores, sells on a dynamic market, completes quests, crafts items, and upgrades their ship before launching the next mission.

---

## UI Structure

The base screen is divided into tabs/panels navigated by clicking buttons:

```
[INVENTORY] [MARKET] [QUESTS] [CRAFTING] [SHIP] [LAUNCH MISSION]
```

All panels are drawn in screen space using raylib's `DrawRectangle`, `DrawText`, `GuiButton` (raygui.h — single-header UI included with raylib extras).

---

## Inventory Panel

Shows all owned resources and money.

```cpp
struct Inventory {
    std::unordered_map<ResourceType, int> resources;
    int money = 500;   // starting money

    void Add(ResourceType r, int amount);
    bool Spend(int cost);
    int  Count(ResourceType r) const;
};
```

Display: grid of resource icons (colored squares) with count labels. Scrollable if many types.

---

## Market Panel

### Dynamic Pricing

Prices shift based on supply/demand simulation:

```cpp
struct MarketEntry {
    ResourceType type;
    int   base_price;     // credits per unit
    float demand;         // 0.5–2.0 multiplier, shifts each "day" (mission)
    float supply_pressure;// increases as player sells more, decreases over time

    int CurrentBuyPrice()  const;  // what player gets when selling
    int CurrentSellPrice() const;  // what player pays when buying
};
```

Price formula:
```
buy_price  = base_price * demand * (1 / (1 + supply_pressure * 0.3))
sell_price = buy_price * 1.25    // market markup
```

Each mission completion: randomize `demand` slightly (±0.2), decay `supply_pressure` by 0.1.
When player sells N units: `supply_pressure += N * 0.02`.

### Market State Persistence

`MarketState` is part of `GameState` — persists across all missions.

### UI

Table: Resource | Current Buy | Current Sell | Qty to trade | [BUY] [SELL] buttons.

---

## Quest Panel

```cpp
struct Quest {
    std::string description;   // "Deliver 20 units of Copper"
    ResourceType resource;
    int  required_amount;
    int  reward_money;
    int  reward_special;       // optional: bonus ammo, drone pack, etc.
    bool completed = false;
    int  missions_left;        // expires after N missions if not completed
};
```

3–5 active quests at a time. New quests generated each mission cycle.

Quest generation:
- Pick random resource (weighted toward rarer ones for better rewards)
- Amount: proportional to base capacity + upgrade level
- Reward: `required_amount * resource_value * 1.5` credits

Completing a quest: player must have the required resources in inventory, clicks [COMPLETE] → resources consumed, money awarded.

---

## Crafting Panel

Converts raw materials into refined goods with higher market value, or into consumable items.

```cpp
struct Recipe {
    std::string name;
    std::unordered_map<ResourceType, int> inputs;
    std::unordered_map<ResourceType, int> outputs;
    int craft_cost;   // money cost (energy/tools)
};
```

### Starting Recipes

| Name | Input | Output | Cost |
|------|-------|--------|------|
| Smelt Iron | 5 Iron | 3 Iron Bar | 20cr |
| Smelt Copper | 5 Copper | 3 Copper Wire | 20cr |
| Silicon Wafer | 4 Silicon | 2 Silicon Wafer | 30cr |
| Titanium Plate | 3 Titanium | 2 Ti Plate | 60cr |
| Gold Ingot | 3 Gold | 2 Gold Ingot | 50cr |
| Drone Repair Kit | 2 Iron Bar + 1 Silicon Wafer | +3 Drones (consumable) | 0cr |
| Shell Casing | 2 Copper Wire + 1 Iron Bar | +20 AP Ammo | 0cr |

Crafting is instant (prototype — no time simulation).

---

## Ship Upgrade Panel

```cpp
struct ShipUpgrade {
    std::string name;
    std::string description;
    int         cost;
    bool        purchased = false;
    // Applied effect is handled in GameState → ship stats
};
```

### Upgrade Tree (flat list for prototype)

| Upgrade | Cost | Effect |
|---------|------|--------|
| Hull Plating I | 300cr | +50 max HP |
| Hull Plating II | 800cr | +100 max HP (req I) |
| Cargo Hold I | 250cr | +50 cargo capacity |
| Cargo Hold II | 600cr | +100 cargo capacity (req I) |
| Engine Boost | 400cr | +100 max speed, +100 thrust |
| FTL Capacitor | 500cr | FTL charge time −2s |
| Better Optics | 350cr | Flashlight radius +100px |
| Extended Mag | 300cr | +50% ammo all limited weapons |
| Fast Loader | 350cr | −20% cooldown all weapons |
| Penetrator Rounds | 450cr | +1 penetration all projectiles |
| Drone Pack | 200cr | +5 drones per mission |
| Flare Pack | 150cr | +8 flares per mission |
| Catcher Capacity | 250cr | 10 simultaneous tether capacity |

Upgrades apply immediately to `GameState.ship_config`. `MissionScene` reads from `ship_config` when spawning the ship.

---

## Death Penalty

When `ship_lost = true` arrives from mission:
```cpp
void GameState::ApplyDeathPenalty() {
    // All mission resources lost (not added to inventory — handled before this call)
    // Ship reverts to default config (all upgrades lost)
    ship_config = ShipConfig::Default();
    // Money is retained (can always buy default ship and supplies)
    // A "default ship" is always available for free from the base
}
```

The default ship has base stats and no weapons beyond slot 1 (regular shot) and slot 7 (catcher).

---

## Launch Mission Button

Opens a small pre-launch summary:
- Current ship stats
- Weapons equipped and ammo counts
- Confirm button → transitions to `MissionScene`
- Seed for the new field is derived from `current_run_number + random_offset`

---

## Persistence (Save/Load)

`GameState` is serialized to a plain text file (`save.dat`) using a simple key=value format:

```
money=1200
resource.gold=8
resource.copper=22
upgrade.hull_plating_1=1
upgrade.cargo_hold_1=1
market.gold.demand=1.3
market.copper.demand=0.9
run_number=4
```

Loaded on game start, written after every base-mode action (market trade, quest complete, upgrade purchase, mission launch).
