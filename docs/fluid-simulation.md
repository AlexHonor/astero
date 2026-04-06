# Dust / Fluid Simulation

## Overview

Destroyed tiles optionally produce **dust** — a semi-transparent gas overlay rendered above everything. Implemented as a sparse grid cellular automaton, rendered via a `RenderTexture2D` each frame.

---

## Grid

```cpp
static constexpr int   DUST_CELL_SIZE = 8;    // world px per dust cell
static constexpr int   DUST_GRID_W    = 500;  // 500 * 8 = 4000 px
static constexpr int   DUST_GRID_H    = 500;
static constexpr float DUST_SIM_HZ    = 10.f; // update rate

struct DustCell {
    float   density = 0.f;    // 0 = empty, 1 = full
    Color   color;
    Vector2 vel;              // drift direction
};
```

Only cells with `density > 0.01f` are active. Track them in an `std::unordered_set<int>` (linear index = row * DUST_GRID_W + col).

---

## Simulation Step (runs at 10 Hz)

```cpp
void DustSim::Step(float dt) {
    std::unordered_set<int> next_active;

    for (int idx : active_cells) {
        int col = idx % DUST_GRID_W;
        int row = idx / DUST_GRID_W;
        DustCell& c = grid[row][col];

        // 1. Decay
        c.density *= 0.97f;

        // 2. Drift — deposit fraction into neighbor in velocity direction
        float deposit = c.density * 0.15f;
        c.density -= deposit;
        int nc = col + (int)roundf(c.vel.x);
        int nr = row + (int)roundf(c.vel.y);
        if (InBounds(nc, nr)) {
            grid[nr][nc].density += deposit;
            grid[nr][nc].color    = c.color;
            grid[nr][nc].vel      = Vector2Scale(c.vel, 0.95f); // slow drift
            next_active.insert(nr * DUST_GRID_W + nc);
        }

        // 3. Diffuse — spread 2% to all 4 neighbors
        float spread = c.density * 0.02f;
        c.density -= spread * 4;
        for (auto [dc, dr] : std::array<std::pair<int,int>,4>{{{1,0},{-1,0},{0,1},{0,-1}}}) {
            if (InBounds(col+dc, row+dr)) {
                grid[row+dr][col+dc].density += spread;
                next_active.insert((row+dr)*DUST_GRID_W+(col+dc));
            }
        }

        if (c.density > 0.01f) next_active.insert(idx);
    }

    active_cells = next_active;
}
```

---

## Adding Dust

Called when a tile is destroyed and integrity check says dust (not chunk):

```cpp
void DustSim::AddDust(Vector2 world_pos, Material mat, Vector2 impulse) {
    int col = (int)(world_pos.x / DUST_CELL_SIZE);
    int row = (int)(world_pos.y / DUST_CELL_SIZE);
    if (!InBounds(col, row)) return;

    DustCell& c = grid[row][col];
    c.density = fminf(1.f, c.density + 0.8f);
    c.color   = MaterialDustColor(mat);
    c.vel     = Vector2Scale(impulse, 0.01f);  // convert px/s impulse to cell/step
    active_cells.insert(row * DUST_GRID_W + col);
}

// For explosions: call AddDust for cells in a circle with outward velocity
void DustSim::AddExplosionDust(Vector2 center, float radius, Material mat) {
    int cell_r = (int)(radius / DUST_CELL_SIZE) + 1;
    Vector2 c_cell = Vector2Scale(center, 1.f/DUST_CELL_SIZE);
    for (int dr = -cell_r; dr <= cell_r; dr++) {
        for (int dc = -cell_r; dc <= cell_r; dc++) {
            if (sqrtf(dr*dr+dc*dc) <= cell_r) {
                Vector2 outward = Vector2Normalize({(float)dc, (float)dr});
                AddDust({(c_cell.x+dc)*DUST_CELL_SIZE, (c_cell.y+dr)*DUST_CELL_SIZE},
                        mat, Vector2Scale(outward, 200.f));
            }
        }
    }
}
```

---

## Rendering

```cpp
void DustSim::Draw(RenderTexture2D& target, Camera2D& cam) {
    BeginTextureMode(target);
    ClearBackground({0,0,0,0});   // transparent
    BeginMode2D(cam);

    for (int idx : active_cells) {
        int col = idx % DUST_GRID_W;
        int row = idx / DUST_GRID_W;
        DustCell& c = grid[row][col];

        Color draw_col = c.color;
        draw_col.a = (unsigned char)(c.density * 180);  // max 70% opacity
        DrawRectangle(col * DUST_CELL_SIZE, row * DUST_CELL_SIZE,
                      DUST_CELL_SIZE, DUST_CELL_SIZE, draw_col);
    }

    EndMode2D();
    EndTextureMode();

    // Blit onto screen with additive-ish blending (BLEND_ALPHA)
    DrawTextureRec(target.texture,
        {0,0,(float)target.texture.width,-(float)target.texture.height},
        {0,0}, WHITE);
}
```

---

## Material Dust Colors

| Material | Dust Color (RGBA) |
|----------|------------------|
| Rock     | `{ 80, 80, 85, 255 }` |
| Iron     | `{ 120, 70, 40, 255 }` |
| Copper   | `{ 70, 130, 80, 255 }` |
| Silicon  | `{ 160, 160, 190, 255 }` |
| Titanium | `{ 60, 90, 130, 255 }` |
| Gold     | `{ 200, 160, 0, 255 }` |

---

## Performance Notes

- Active cell cap: 8,000. If exceeded, oldest cells (lowest density) decay 3× faster.
- `RenderTexture2D` is only rebuilt if active_cells changed since last draw.
- Dust does not block bullets or physics.
- Dense dust (density > 0.7 at ship cell) halves the drone scan reveal distance.
