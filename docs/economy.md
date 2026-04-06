# Economy & Roguelite Loop

## Resources

| Resource | Raw Value (cr/unit) | Refined Form | Refined Value |
|----------|--------------------|--------------|--------------:|
| Rock | 0 | — | — |
| Iron | 5 | Iron Bar | 12 |
| Copper | 15 | Copper Wire | 35 |
| Silicon | 12 | Silicon Wafer | 30 |
| Titanium | 40 | Titanium Plate | 95 |
| Gold | 80 | Gold Ingot | 190 |

Refining requires crafting (see base-mode.md). Selling raw is always possible but less efficient.

---

## Starting State

New game / post-death defaults:
- Money: 500cr
- Ship: Default (100 HP, 50 cargo, slots 1 + 7 only)
- No resources
- Market: all demands at 1.0

500cr is enough to buy Extended Mag or one Hull Plating I after a single good mission.

---

## Roguelite Progression

```
Run 1 → Collect resources → Sell/Craft → Upgrade → Run 2 → ...
```

**Persists across runs (even after death):**
- Money
- Crafted/purchased consumables stored in base inventory
- Purchased ship upgrades lost on death, but money kept

**Lost on death:**
- Resources collected on that mission (never made it back)
- All ship upgrades (ship is destroyed)
- Equipped consumable ammo beyond defaults

The death sting is losing the ship investment. Money surviving means players aren't completely reset — they can always buy back a default ship and start climbing again.

---

## Difficulty Scaling

Each run number increases field difficulty:

```cpp
FieldConfig MakeConfig(int run_number) {
    FieldConfig cfg;
    cfg.num_asteroids = 10 + run_number * 2;           // more asteroids
    cfg.max_size      = 40 + run_number * 3;           // bigger asteroids
    cfg.min_size      = 15 + run_number;
    // Richer fields: increase probability of deep-material veins
    cfg.deep_material_bias = 0.1f + run_number * 0.05f;
    return cfg;
}
```

This means early runs are sparser (easier to navigate, less reward) and later runs are denser and more lucrative.

---

## Market Volatility

Each mission completion shuffles demand:
```cpp
for (auto& [type, entry] : market) {
    float shift = GetRandomFloat(-0.25f, 0.25f);
    entry.demand = Clamp(entry.demand + shift, 0.4f, 2.5f);
    entry.supply_pressure *= 0.7f;   // supply pressure decays
}
```

This means prices fluctuate meaningfully between runs. Sometimes gold crashes (demand 0.5), making copper more valuable. Players should watch market trends.

---

## Quest Rewards Scaling

Quest rewards scale with run number:
```
reward = base_resource_value * amount_required * (1.2 + run_number * 0.1)
```

Run 1 quests are simple (10 copper, ~150cr reward). Run 10 quests demand larger quantities with proportionally higher rewards.

---

## Economy Balance Targets

| Milestone | Run # | Expected |
|-----------|-------|---------|
| First upgrade purchased | 1–2 | Hull Plating I or Cargo Hold I |
| Full ship loadout | 6–8 | All upgrades purchased |
| Max cargo / max speed | 4–6 | Core upgrades done |
| Sustainable gold farming | 8+ | AP rounds + FTL capacitor for efficient deep runs |

These are loose guidelines for playtesting balance, not hard constraints.
