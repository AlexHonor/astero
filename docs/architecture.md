# Architecture

## Tech Stack

| Layer | Choice | Reason |
|-------|--------|--------|
| Language | C++17 | Performance, control |
| Graphics/Input/Audio | raylib 5.x | Minimal, no engine overhead, simple API |
| Build | CMake 3.20+ | Standard, cross-platform |
| Physics | Custom (no lib) | Only need grid splitting + simple velocity; no full physics needed |
| Math | raylib's `raymath.h` | Vector2, Matrix, lerp — already bundled |

No external dependencies beyond raylib. Everything else is hand-rolled and kept simple.

---

## Project Structure

```
asteroid-rush/
├── CMakeLists.txt
├── cmake/
│   └── FetchRaylib.cmake
├── src/
│   ├── main.cpp
│   ├── core/
│   │   ├── game.h / game.cpp           # Main game loop, scene switching
│   │   ├── game_state.h / .cpp         # Persistent state (resources, money, ship)
│   │   ├── input.h / .cpp              # Input abstraction
│   │   └── save.h / .cpp               # JSON-style save/load (hand-rolled)
│   ├── mission/
│   │   ├── mission_scene.h / .cpp      # Top-level mission orchestration
│   │   ├── ship.h / .cpp               # Player ship
│   │   ├── asteroid.h / .cpp           # Single asteroid body
│   │   ├── asteroid_generator.h / .cpp # Procedural field generation
│   │   ├── tile.h                      # Tile data struct
│   │   ├── weapon_manager.h / .cpp     # Weapon slots, firing
│   │   ├── weapons/
│   │   │   ├── projectile.h / .cpp     # Base projectile
│   │   │   ├── explosive.h / .cpp
│   │   │   ├── catcher.h / .cpp
│   │   │   └── flare.h / .cpp
│   │   ├── chunk.h / .cpp              # Free-floating mineral chunk
│   │   ├── intel_drone.h / .cpp
│   │   ├── fog_of_war.h / .cpp
│   │   ├── dust_sim.h / .cpp           # Grid-based dust fluid sim
│   │   ├── lighting.h / .cpp           # Light mask render texture
│   │   └── hud.h / .cpp
│   └── base/
│       ├── base_scene.h / .cpp
│       ├── inventory.h / .cpp
│       ├── market.h / .cpp
│       ├── quests.h / .cpp
│       ├── crafting.h / .cpp
│       └── base_hud.h / .cpp
├── assets/                             # Empty for now — prototype uses shapes only
└── docs/
```

---

## CMakeLists.txt (root)

```cmake
cmake_minimum_required(VERSION 3.20)
project(AsteroidRush VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/FetchRaylib.cmake)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(asteroid_rush ${SOURCES})

target_include_directories(asteroid_rush PRIVATE src)
target_link_libraries(asteroid_rush PRIVATE raylib)

# Windows: link required system libs
if(WIN32)
    target_link_libraries(asteroid_rush PRIVATE winmm)
endif()
```

## cmake/FetchRaylib.cmake

```cmake
include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    TRUE
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)
```

---

## Game Loop

```cpp
// main.cpp
int main() {
    InitWindow(1280, 720, "Asteroid Rush");
    SetTargetFPS(60);

    Game game;
    game.Init();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        game.Update(dt);

        BeginDrawing();
        ClearBackground(BLACK);
        game.Draw();
        EndDrawing();
    }

    game.Shutdown();
    CloseWindow();
    return 0;
}
```

```cpp
// game.h — scene enum + switcher
enum class Scene { MAIN_MENU, MISSION, BASE };

class Game {
public:
    void Init();
    void Update(float dt);
    void Draw();
    void Shutdown();
    void SwitchScene(Scene s);

private:
    Scene current_scene;
    GameState state;          // persists across scene switches
    // owned scene objects
    std::unique_ptr<MissionScene> mission;
    std::unique_ptr<BaseScene>    base;
};
```

---

## Coordinate System

- World space: pixels, origin top-left (raylib default)
- Camera: `Camera2D` (raylib built-in) centered on ship, supports zoom/pan
- Tile grid: each tile = 16×16 px in world space
- Play area: 4000×4000 px world space

```cpp
// Convert world ↔ tile
Vector2i WorldToTile(Vector2 world) {
    return { (int)(world.x / TILE_SIZE), (int)(world.y / TILE_SIZE) };
}
Vector2 TileToWorld(Vector2i tile) {
    return { tile.x * TILE_SIZE + TILE_SIZE * 0.5f,
             tile.y * TILE_SIZE + TILE_SIZE * 0.5f };
}
```

---

## Scene Transitions

`Game::SwitchScene()` is called with a result payload:

```cpp
struct MissionResult {
    bool player_died;
    std::unordered_map<ResourceType, int> resources_collected;
    float ship_hp_fraction;
};
```

On FTL: `game.SwitchScene(Scene::BASE)` with full `MissionResult`.  
On death: same call, `player_died = true`, resources empty.

`GameState` merges the result, then `BaseScene` is initialized fresh.

---

## Rendering Order (DrawOrder Z)

Draw calls issued in this order each frame:

1. `BeginMode2D(camera)` — everything below is in world space
2. Asteroid tilemaps (colored rectangles per tile)
3. Collectible chunks (small colored squares/circles)
4. Ship and projectiles
5. Intel drone bodies
6. `EndMode2D()`
7. Lighting overlay (RenderTexture blended onto screen — see lighting.md)
8. Dust overlay (RenderTexture blended onto screen — see fluid-simulation.md)
9. Fog of war overlay (RenderTexture per asteroid)
10. `BeginMode2D(camera)` again for HUD-world elements (scan radius rings, etc.)
11. `EndMode2D()`
12. HUD (screen space — no camera)

---

## Graphics Style

Pure raylib primitives — no textures for prototype:

| Object | Shape | Color |
|--------|-------|-------|
| Ship | `DrawPoly` (triangle) | WHITE |
| Asteroid tile | `DrawRectangle` | Material color (see asteroid-system.md) |
| Bullet | `DrawCircle` (r=3) | YELLOW |
| Explosive shell | `DrawCircle` (r=5) | ORANGE |
| Chunk | `DrawRectangle` (8×8) | Material color, 50% brighter |
| Intel drone | `DrawPoly` (diamond, 4 sides) | SKYBLUE |
| Flare (active) | `DrawCircle` (r=4) | ORANGE + glow effect |
| Explosion | Expanding `DrawCircleLines` rings, fading alpha | ORANGE→RED |
| HUD bars | `DrawRectangle` fills | GREEN / RED / BLUE |
