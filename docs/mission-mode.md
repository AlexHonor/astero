# Mission Mode

## Overview

Real-time session in a dark asteroid field. Pilot the ship, mine resources, escape via FTL. Death = lose everything carried. Survive and collect as much as possible.

---

## Ship

```cpp
class Ship {
public:
    Vector2 pos;
    Vector2 vel;
    float   rotation;       // radians, faces mouse cursor
    int     hp, max_hp;
    int     cargo, max_cargo;

    void Update(float dt, Camera2D& cam);
    void Draw();
    void TakeDamage(int dmg);
    bool IsDead() const { return hp <= 0; }

    WeaponManager weapons;
};
```

### Movement

Arrow keys / WASD: apply thrust to velocity. Velocity damped each frame.

```cpp
void Ship::Update(float dt, Camera2D& cam) {
    Vector2 thrust = {0, 0};
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    thrust.y -= 1;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  thrust.y += 1;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  thrust.x -= 1;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) thrust.x += 1;

    thrust = Vector2Normalize(thrust);   // no diagonal speed bonus
    vel = Vector2Add(vel, Vector2Scale(thrust, THRUST * dt));
    vel = Vector2Scale(vel, 1.f - DAMPING * dt);  // DAMPING ~ 2.5
    vel = Vector2ClampValue(vel, 0, MAX_SPEED);

    pos = Vector2Add(pos, Vector2Scale(vel, dt));

    // Face mouse
    Vector2 mouse_world = GetScreenToWorld2D(GetMousePosition(), cam);
    rotation = Vector2Angle({1,0}, Vector2Subtract(mouse_world, pos));
}
```

Ship is drawn as a triangle (`DrawPoly` with 3 sides) pointing in `rotation` direction.

### Chunk Collection (contact)

Any free chunk within 20px of the ship is auto-collected if cargo has space:
```cpp
for (auto& chunk : chunks) {
    if (Vector2Distance(chunk.pos, ship.pos) < 20.f && ship.cargo < ship.max_cargo) {
        ship.cargo++;
        game_state.AddResource(chunk.material, 1);
        chunk.alive = false;
    }
}
```

### Collision with Asteroids

Check ship AABB vs. each asteroid's tile AABB. On overlap:
```cpp
float impact_speed = Vector2Length(Vector2Subtract(ship.vel, asteroid_vel_at_point));
if (impact_speed > 20.f) {
    ship.TakeDamage((int)(impact_speed * 0.1f));
    ship.vel = Vector2Scale(ship.vel, -0.3f);  // bounce back
}
```

---

## FTL Escape

```cpp
class FTLSystem {
public:
    bool  charging = false;
    float charge_progress = 0.f;   // 0–1
    float charge_time     = 8.f;   // seconds (upgradeable)
    bool  on_cooldown     = false;
    float cooldown_left   = 0.f;

    void TryActivate();
    void Update(float dt, Ship& ship);
    bool IsComplete() const { return charge_progress >= 1.f; }
};

void FTLSystem::Update(float dt, Ship& ship) {
    if (on_cooldown) {
        cooldown_left -= dt;
        if (cooldown_left <= 0) on_cooldown = false;
        return;
    }
    if (!charging) return;

    // Interrupted by damage? (checked in Ship::TakeDamage → calls Interrupt())
    charge_progress += dt / charge_time;
    charge_progress = fminf(charge_progress, 1.f);
}
```

HUD shows a vertical bar filling up during charge. While charging, ship velocity is set to 0.

---

## Mission Flow

```
Init:
  AsteroidGenerator::Generate(cfg, seed) → std::vector<Asteroid>
  Ship spawned at {200, world_height/2}
  LightingSystem initialized, ship flashlight registered
  DustSim initialized
  Minimap RenderTexture created

Per-frame Update:
  ship.Update(dt)
  for asteroid: asteroid.Update(dt)      // drift + rotate
  weapons.Update(dt)                     // move projectiles, check hits
  CollectChunksInRange()
  ftl.Update(dt, ship)
  dust.Step() [at 10 Hz]
  lighting.Update(dt)
  CheckBoundaries()
  if (ship.IsDead()) → EndMission(died=true)
  if (ftl.IsComplete()) → EndMission(died=false)

Per-frame Draw:
  BeginMode2D(cam)
    for asteroid: asteroid.Draw()
    for chunk: chunk.Draw()
    weapons.Draw()
    ship.Draw()
    for drone: drone.Draw()
  EndMode2D()
  lighting.Draw(cam)      // light mask overlay
  dust.Draw(rtex, cam)    // dust overlay
  hud.Draw(ship, ftl)     // screen space
```

---

## HUD

All drawn in screen space after `EndMode2D()`:

| Element | Position | Draw call |
|---------|----------|-----------|
| HP bar | (10, 10), 200×16 | `DrawRectangle` green fill + red remainder |
| Cargo bar | (10, 32) | `DrawRectangle` blue fill |
| Weapon slots | bottom-center | 9 rectangles, active = WHITE border, others = GRAY |
| Ammo count | below each slot | `DrawText` |
| FTL button | (sw-120, sh-40) | Rectangle, fills CYAN during charge |
| Minimap | (sw-210, 10), 200×200 | Blit minimap RenderTexture |
| Drone count | (sw-210, 215) | `DrawText` |
| Timer | (sw/2-30, 10) | `DrawText` |
| Scan popup | (sw-250, 230) | `DrawText` lines, fades with timer |
| Boundary warning | screen edges | Flashing RED rectangles |

---

## Play Area & Camera

- World: 4000×4000 px
- `Camera2D` centered on ship, smooth follow:
  ```cpp
  cam.target = Vector2Lerp(cam.target, ship.pos, 5.f * dt);
  cam.offset = {GetScreenWidth()/2.f, GetScreenHeight()/2.f};
  ```
- Scroll wheel zoom: `cam.zoom = Clamp(cam.zoom + wheel*0.1f, 0.4f, 2.5f)`
- Boundary: ship takes 5 dmg/s outside world rect. HUD flashes red at edges.
