# Implementation Checklist

Step-by-step build order. Each step should compile and run before moving to the next.
Work through these in order — later steps depend on earlier ones.

---

## Phase 1 — Project Skeleton

- [ ] **1.1** Create CMakeLists.txt + cmake/FetchRaylib.cmake, verify `cmake --build` downloads raylib and compiles a blank window
- [ ] **1.2** Create `src/main.cpp` with game loop (InitWindow, game loop, CloseWindow)
- [ ] **1.3** Create `Game` class (game.h/cpp) with `Init`, `Update`, `Draw`, `Shutdown`, scene enum
- [ ] **1.4** Create `GameState` singleton (game_state.h/cpp) — money, resource map, ship config stub
- [ ] **1.5** Create `SaveSystem` (save.h/cpp) — write/read key=value text file, hook into GameState
- [ ] **1.6** Stub out `MissionScene` and `BaseScene` classes; `Game::SwitchScene()` instantiates them

---

## Phase 2 — Asteroid System

- [ ] **2.1** Define `Tile` struct and `Material` enum (tile.h)
- [ ] **2.2** Implement `Asteroid` class with a hand-authored 10×10 test grid; render tiles as colored rectangles
- [ ] **2.3** Implement `Camera2D` setup in `MissionScene`; camera follows a fixed point; confirm zoom/pan works
- [ ] **2.4** Implement `AsteroidGenerator` — polar noise shape, depth field, material assignment, cavity punching
- [ ] **2.5** Spawn a field of 8–12 asteroids at random positions; verify varied shapes and material layering
- [ ] **2.6** Implement asteroid drift and rotation (velocity + angular_vel applied each frame)
- [ ] **2.7** Implement `IsTileVisible()` fog-of-war per tile; hidden tiles draw as dark silhouette
- [ ] **2.8** Implement `Asteroid::DamageTile()` — reduce HP, destroy tile when HP ≤ 0, armor/penetration check
- [ ] **2.9** Implement island BFS (`GetIslands()`) and `CheckSplit()` — splitting asteroid on tile destruction
- [ ] **2.10** Implement `Chunk` class — free-floating colored rectangle, slow random drift
- [ ] **2.11** Wire destruction → spawn chunk or dust placeholder (no dust sim yet — just spawn chunks for now)

---

## Phase 3 — Ship & Input

- [ ] **3.1** Implement `Ship` class — triangle drawing, WASD/arrow thrust, velocity damping, rotation toward mouse
- [ ] **3.2** Camera smooth-follow ship
- [ ] **3.3** Ship–asteroid AABB collision (push ship out, apply velocity damage)
- [ ] **3.4** Cargo collection on chunk contact — auto-collect within range, update cargo counter
- [ ] **3.5** HUD skeleton — HP bar, cargo bar, timer (hardcoded values first, wire to ship data after)

---

## Phase 4 — Weapons

- [ ] **4.1** Implement `Projectile` base class — pos, vel, lifetime, `Update()`, `Draw()` (circle)
- [ ] **4.2** Implement `WeaponManager` — 9 slots, key 1–9 switch, cooldown tracking, `Fire()` call
- [ ] **4.3** Weapon 1: Regular Shot — basic projectile, DDA ray vs asteroid tile grid, `DamageTile()` on hit
- [ ] **4.4** Weapon 7: Catcher — pass-through projectile, tether chunks in range, pull to ship
- [ ] **4.5** Weapon 2: Armor-Piercing Shot — same as regular but higher penetration/speed, lower damage
- [ ] **4.6** Weapon 4: Explosive Shell — on-hit area explosion, `ExplosionDamage()` on asteroid
- [ ] **4.7** Weapon 3: Shrapnel Bullet — fuse timer, split into 6 fragments
- [ ] **4.8** Weapon 5: Cluster Bomb — arm timer, split into 5 sub-munitions
- [ ] **4.9** Weapon 6: Timed Mine — embeds on surface, countdown, right-click detonation
- [ ] **4.10** Add ammo counts to HUD weapon slots
- [ ] **4.11** Wire weapon upgrades from `GameState.ship_config` (ammo multiplier, cooldown multiplier, pen bonus)

---

## Phase 5 — Fog of War & Intel Drone

- [ ] **5.1** Ensure `IsTileVisible()` renders correctly — surface tiles show material, interior tiles show fog color
- [ ] **5.2** Implement `IntelDrone` — fires as slow projectile, lands on asteroid surface
- [ ] **5.3** Drone scan: `reveal_area()` marks tiles within 150px as `revealed = true`
- [ ] **5.4** `ScanReport` — count materials in scan radius, display in HUD as popup with timer
- [ ] **5.5** Weapon 9 (Intel Drone) wired into `WeaponManager`
- [ ] **5.6** Minimap `RenderTexture2D` — draw asteroid tiles as dots, ship as white dot, update at 10 Hz

---

## Phase 6 — Lighting

- [ ] **6.1** Implement `LightingSystem` — `RenderTexture2D` mask, `AddLight`/`MoveLight`/`RemoveLight`
- [ ] **6.2** Bake radial gradient texture at startup
- [ ] **6.3** Ship flashlight: permanent light, moves with ship toward mouse each frame
- [ ] **6.4** Apply mask over screen with `BLEND_MULTIPLIED` — confirm world goes dark except flashlight cone
- [ ] **6.5** Weapon 8: Flare — fires, lands on surface, `AddLight()` to `LightingSystem` with 15s lifetime + flicker
- [ ] **6.6** `LightingSystem::Update()` — decay flare lifetimes, `RemoveLight()` when expired

---

## Phase 7 — Dust Simulation

- [ ] **7.1** Implement `DustSim` — sparse grid, `AddDust()`, `Step()` at 10 Hz
- [ ] **7.2** Wire tile destruction → `AddDust()` when integrity roll fails
- [ ] **7.3** Wire `ExplosionDamage()` → `AddExplosionDust()` for destroyed tiles
- [ ] **7.4** Render dust to `RenderTexture2D`, blit to screen after lighting layer
- [ ] **7.5** Dense dust halves drone scan radius (check ship cell density in drone scan)

---

## Phase 8 — FTL & Mission End

- [ ] **8.1** Implement `FTLSystem` — charge progress, cooldown, interrupt on damage
- [ ] **8.2** FTL HUD — charge bar, button
- [ ] **8.3** On charge complete: package `MissionResult` from `ship.cargo` + `game_state`, call `Game::SwitchScene(BASE)`
- [ ] **8.4** On ship death: `MissionResult` with `player_died = true`, call `SwitchScene(BASE)`
- [ ] **8.5** Play area boundary: ship takes damage outside 4000×4000 rect, screen edge warning

---

## Phase 9 — Base Mode

- [ ] **9.1** Implement `BaseScene` with tab navigation (Inventory / Market / Quests / Crafting / Ship / Launch)
- [ ] **9.2** Inventory panel — display all resources and money from `GameState`
- [ ] **9.3** Apply `MissionResult` on entry — add resources, apply death penalty if needed
- [ ] **9.4** Market panel — display prices, BUY/SELL buttons, update `supply_pressure` on sell
- [ ] **9.5** Market price shuffle on each return from mission
- [ ] **9.6** Quest panel — generate 3–5 quests, display requirements, [COMPLETE] button consumes resources + awards money
- [ ] **9.7** Crafting panel — display recipes, [CRAFT] button validates inputs + executes transformation
- [ ] **9.8** Ship upgrade panel — display upgrade tree, [BUY] button checks money, marks purchased, applies to `ship_config`
- [ ] **9.9** Launch Mission button — pre-launch summary, seed new field, switch to `MissionScene`
- [ ] **9.10** Death penalty on `ship_lost`: reset ship_config to default, display "SHIP LOST" message in base

---

## Phase 10 — Save / Load & Polish

- [ ] **10.1** `SaveSystem::Save()` called after every base action
- [ ] **10.2** `SaveSystem::Load()` called on game startup; restore full `GameState`
- [ ] **10.3** Main menu scene — New Game / Continue buttons
- [ ] **10.4** Difficulty scaling in `AsteroidGenerator::Generate()` based on `run_number`
- [ ] **10.5** Market demand randomization wired to mission-return event
- [ ] **10.6** Quest scaling with `run_number`
- [ ] **10.7** Explosion visual feedback — expanding ring animation (`DrawCircleLines`, fading alpha)
- [ ] **10.8** Ship damage flash (briefly tint ship RED on hit)
- [ ] **10.9** Screen shake on nearby explosions (offset `cam.offset` by small random vector, decay over 0.3s)
- [ ] **10.10** Basic sound effects using `raylib PlaySound()` — fire, explosion, collect, FTL charge

---

## Phase 11 — Testing & Balance

- [ ] **11.1** Playtest: can you get enough gold in one run to buy Hull Plating I?
- [ ] **11.2** Playtest: is the default ship survivable for 3+ minutes without upgrades?
- [ ] **11.3** Balance weapon ammo counts — no weapon should run out in a 5-minute mission under normal play
- [ ] **11.4** Balance asteroid density — player should be able to collect 30–60 cargo units in a 5-minute mission
- [ ] **11.5** Balance market prices — selling a full cargo run (50 units mixed) should net ~400–800cr on run 1
- [ ] **11.6** Fix any split/island detection performance issues on large asteroids
- [ ] **11.7** Verify save/load round-trip preserves all state correctly

---

## Out of Scope for Prototype (Future)

- Enemy ships / combat AI
- Animated ship sprites / tilemap textures
- Sound design beyond simple beeps
- Multiplayer
- More than one asteroid field biome
- Story / dialogue
