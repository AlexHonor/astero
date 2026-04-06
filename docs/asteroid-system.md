# Asteroid System

## Tile Data

Each tile is a plain struct stored in a 2D grid inside `Asteroid`:

```cpp
// tile.h
enum class Material { None, Rock, Iron, Copper, Silicon, Titanium, Gold };

struct Tile {
    Material  material   = Material::None;
    int       hp         = 0;
    int       armor      = 0;       // min bullet penetration for HP damage
    float     exp_resist = 0.f;     // fraction of explosion damage blocked
    float     integrity  = 0.5f;    // chance destroyed tile → chunk (vs dust)
    float     ricochet   = 0.1f;    // chance bullet bounces when pen < armor
    bool      revealed   = false;   // seen by drone scan or surface-adjacent
};
```

### Material Stats Table

| Material | HP | Armor | ExpResist | Integrity | Ricochet | Value | Depth |
|----------|----|-------|-----------|-----------|----------|-------|-------|
| Rock     | 20 | 0 | 0.10 | 0.30 | 0.10 | 0  | outer |
| Iron     | 40 | 1 | 0.20 | 0.50 | 0.20 | 5  | outer-mid |
| Copper   | 60 | 2 | 0.30 | 0.60 | 0.30 | 15 | mid |
| Silicon  | 50 | 1 | 0.15 | 0.70 | 0.20 | 12 | mid |
| Titanium |120 | 4 | 0.60 | 0.40 | 0.50 | 40 | deep |
| Gold     | 80 | 3 | 0.40 | 0.50 | 0.35 | 80 | core |

### Material Colors (raylib Color)

| Material | Color |
|----------|-------|
| Rock     | `DARKGRAY` |
| Iron     | `{ 140, 80, 50, 255 }` (rust brown) |
| Copper   | `{ 80, 160, 90, 255 }` (green-brown) |
| Silicon  | `{ 170, 170, 200, 255 }` (light blue-grey) |
| Titanium | `{ 70, 100, 140, 255 }` (blue-grey) |
| Gold     | `GOLD` |

---

## Asteroid Class

```cpp
// asteroid.h
struct AsteroidGrid {
    std::vector<std::vector<Tile>> cells;  // [row][col]
    int  cols, rows;
    Vector2 world_origin;   // top-left corner in world space
    float rotation;         // radians — asteroid slowly rotates
    float angular_vel;      // rad/s
    Vector2 velocity;       // drift velocity px/s
};

class Asteroid {
public:
    AsteroidGrid grid;

    void Update(float dt);
    void Draw(Camera2D& cam);
    void DamageTile(int col, int row, int dmg, int pen);
    void ExplosionDamage(Vector2 center, float radius, int dmg);
    bool IsAlive() const;   // at least one non-None tile remains

private:
    void CheckSplit();      // BFS island detection after tile removal
    void SpawnChunk(Vector2 pos, Material mat, Vector2 impulse);
    void SpawnDust(Vector2 pos, Material mat, Vector2 impulse);
    bool IsConnected();     // flood fill check
    std::vector<std::vector<int>> GetIslands();  // returns cell groups
    void RebuildCollider(); // recompute outline for hit detection
};
```

---

## Procedural Generation

`AsteroidGenerator` creates the asteroid field at mission start.

### Single Asteroid Shape — Layered Noise

We implement a simple value noise (no external lib):

```cpp
// Step 1: generate a bool mask via polar noise
// For each angle θ and radius r: cell is solid if r < noisy_radius(θ)
// noisy_radius(θ) = base_r * (1 + amplitude * noise(θ * frequency))

// Step 2: fill a grid — cell (col, row) is solid if its center
// falls within the polar mask

// Step 3: compute depth field
// depth[col][row] = distance to nearest empty cell / max_depth
// (simple BFS from all border/empty cells inward)

// Step 4: assign material by depth
Material MaterialAtDepth(float depth, uint32_t seed_mix) {
    // depth 0.0–0.2 → Rock
    // depth 0.2–0.5 → Iron or Copper (random)
    // depth 0.5–0.75 → Copper or Silicon or Titanium
    // depth 0.75–1.0 → Titanium or Gold
    // Vein injection: secondary noise can force Gold/Titanium in mid-depth
}

// Step 5: punch cavities — randomly zero out interior clusters
```

### Field Generation

```cpp
struct FieldConfig {
    int   num_asteroids = 12;     // total count
    float field_radius  = 1600.f; // spawn within this radius of center
    float min_dist      = 200.f;  // minimum separation between asteroids
    int   min_size      = 18;     // tiles
    int   max_size      = 64;     // tiles
};

void AsteroidGenerator::Generate(FieldConfig cfg, uint32_t seed) {
    // Place asteroids with rejection sampling (min_dist enforced)
    // Each gets a unique sub-seed derived from master seed + index
    // Each gets a small random velocity + angular velocity
}
```

---

## Physics

Asteroids are **not** full rigid bodies. They use:
- `velocity` (Vector2, px/s) — updated each frame: `pos += vel * dt`
- `rotation` / `angular_vel` — rotates the grid visually

No collision between asteroids in prototype (they pass through each other). Collision with the player ship is AABB against tile grid.

### Split Logic

Run after any tile destruction:

```cpp
void Asteroid::CheckSplit() {
    auto islands = GetIslands();   // BFS, returns groups of {col,row}
    if (islands.size() <= 1) return;

    // Largest island stays in this Asteroid
    // Each smaller island:
    //   if size <= MIN_CHUNK_GROUP (3 tiles): emit chunks/dust for each tile
    //   else: create new Asteroid with those tiles, give outward impulse
}
```

`GetIslands()`: standard 4-connected flood fill on non-None cells. O(N) where N = tile count.

---

## Hit Detection

Ray–tile intersection for bullets:
```cpp
// Cast ray from bullet position along velocity direction
// Step through tiles along the ray using DDA (digital differential analysis)
// First solid tile hit = collision
```

Area damage (explosions):
```cpp
// For each tile within circle: compute overlap, apply scaled damage
for each tile in bounding box of explosion circle:
    if circle contains tile center:
        float dist_factor = 1.0f - (dist / radius);
        DamageTile(col, row, base_dmg * dist_factor * (1 - tile.exp_resist), pen);
```

---

## Drawing

Each tile is drawn as a 16×16 rectangle at its world position (accounting for asteroid rotation around grid center):

```cpp
void Asteroid::Draw(Camera2D& cam) {
    Vector2 center = GridCenter();
    for (int r = 0; r < grid.rows; r++) {
        for (int c = 0; c < grid.cols; c++) {
            Tile& t = grid.cells[r][c];
            if (t.material == Material::None) continue;

            Vector2 local = { (c - grid.cols/2.f) * TILE_SIZE,
                              (r - grid.rows/2.f) * TILE_SIZE };
            Vector2 rotated = Vector2Rotate(local, grid.rotation);
            Vector2 world = Vector2Add(center, rotated);

            Color col = MaterialColor(t.material);
            if (!t.revealed) col = ColorBrightness(DARKGRAY, -0.5f); // fog
            DrawRectangleV(Vector2SubtractValue(world, TILE_SIZE/2), 
                           {TILE_SIZE, TILE_SIZE}, col);
        }
    }
}
```
