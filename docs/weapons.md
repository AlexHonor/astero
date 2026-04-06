# Weapon System

## Penetration vs. Armor

On tile hit:

```cpp
void Projectile::HitTile(Tile& tile) {
    if (penetration < tile.armor) {
        if (GetRandomFloat() < tile.ricochet) {
            Ricochet();   // reflect velocity, lose 30% speed
        }
        // else: absorbed — no damage, projectile dies
        Kill();
        return;
    }
    tile.hp -= damage;
    Kill();
}
```

---

## Weapon Slot

```cpp
struct WeaponSlot {
    int   ammo;          // -1 = unlimited
    float cooldown;      // max cooldown seconds
    float cooldown_left; // current remaining
    bool  CanFire() const { return cooldown_left <= 0 && ammo != 0; }
};
```

`WeaponManager` holds 9 slots, active slot switches on keys 1–9. All slots cooldown in parallel (no shared global cooldown).

---

## Weapon Definitions

### 1 — Regular Shot
- Penetration: 1 | Damage: 15 | Speed: 600 | Cooldown: 0.2s | Ammo: ∞
- Single `DrawCircle` bullet (r=3, YELLOW)

### 2 — Armor-Piercing Shot
- Penetration: 4 | Damage: 10 | Speed: 800 | Cooldown: 0.4s | Ammo: 60
- Thin fast `DrawCircle` (r=2, WHITE)

### 3 — Shrapnel Bullet
- Fires one primary (pen 1, dmg 5). After `fuse_time = 0.5s`, explodes into 6 fragments (pen 0, dmg 8 each) spread ±30°.
- Cooldown: 0.6s | Ammo: 30
- Primary: `DrawCircle` (r=4, ORANGE). Fragments: `DrawCircle` (r=2, ORANGE)

### 4 — Explosive Shell
- On impact: circular explosion, radius 80px, damage 40 (reduced by `exp_resist`).
- Penetration: 2 | Direct dmg: 20 | Cooldown: 1.0s | Ammo: 20
- `DrawCircle` (r=5, ORANGE). Explosion: expanding `DrawCircleLines` rings

### 5 — Cluster Bomb
- After `arm_time = 0.8s`, splits into 5 sub-munitions (pen 1, exp radius 40px, dmg 25 each).
- Cooldown: 1.5s | Ammo: 15
- `DrawCircle` (r=6, RED). Sub-munitions: smaller circles

### 6 — Timed Mine
- Fires slow projectile (speed 150) that embeds in first surface hit.
- After `fuse = 2.0s`, explodes (radius 120px, dmg 60, pen 3).
- Right-click detonates all placed mines early.
- Cooldown: 2.0s | Ammo: 10 | Max placed at once: 5
- Embedded: blinking `DrawCircle` (r=5, RED)

### 7 — Catcher
- Projectile passes through tiles, does zero damage.
- Scans for free chunks within 400px radius along trajectory.
- Attaches tether to up to 5 chunks simultaneously; pulls them toward ship at 150px/s.
- Cooldown: 0.3s | Ammo: ∞
- `DrawLine` tether visualization (SKYBLUE)

### 8 — Flare
- Slow projectile (speed 200). On surface impact or after 3s, embeds and emits light.
- Light radius 200px, duration 15s. Does 0 damage.
- Cooldown: 1.0s | Ammo: 8
- `DrawCircle` (r=4, ORANGE). When active: adds light source to LightingSystem

### 9 — Intel Drone
- Slow projectile (speed 200). On surface impact, attaches to asteroid.
- Reveals fog-of-war in 150px radius. Logs material breakdown to HUD.
- Does 0 damage.
- Cooldown: 3.0s | Ammo: 5
- `DrawPoly` diamond (SKYBLUE) while flying; small square when attached

---

## Projectile Base

```cpp
class Projectile {
public:
    Vector2  pos;
    Vector2  vel;
    int      penetration;
    int      damage;
    float    lifetime;       // auto-kill after this many seconds
    bool     alive = true;

    virtual void Update(float dt);
    virtual void Draw();
    virtual void OnHitTile(Asteroid& ast, int col, int row) {}
    virtual void OnHitNothing() {}   // reached lifetime without hit
    void Kill() { alive = false; }
};
```

Specialized weapons (`ExplosiveShell`, `ShrapnelBullet`, etc.) override `OnHitTile`.

---

## WeaponManager

```cpp
class WeaponManager {
public:
    void Update(float dt);
    void Fire(Vector2 origin, Vector2 direction);
    void HandleInput();         // keys 1–9 switch active_slot
    void Draw();

    std::vector<std::unique_ptr<Projectile>> projectiles;
    std::array<WeaponSlot, 9>  slots;
    int active_slot = 0;        // 0-indexed

    // reference set by MissionScene
    std::vector<Asteroid*>* asteroids;
    std::vector<Chunk*>*    chunks;
    DustSim*                dust;
    LightingSystem*         lighting;
};
```

---

## Upgrades (Base Mode)

| Upgrade | Effect |
|---------|--------|
| Extended Mag | +50% ammo all limited weapons |
| Fast Loader | −20% cooldown all weapons |
| Penetrator Rounds | +1 pen all projectiles |
| Extra Shrapnel | 10 fragments instead of 6 |
| Catcher Capacity | 10 simultaneous tethers |
| Drone Pack | +5 drones |
| Flare Pack | +8 flares |
