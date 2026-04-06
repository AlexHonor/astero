# Fog of War & Intel Drones

## Visibility Rules

| Condition | State |
|-----------|-------|
| Tile has at least one empty neighbor (surface-adjacent) | Visible — drawn in material color |
| Tile is fully surrounded by other solid tiles | Hidden — drawn as dark silhouette (DARKGRAY, darkened) |
| Tile revealed by drone scan | Visible permanently — drawn in material color |
| Tile destroyed | Not drawn |

Surface adjacency is computed dynamically. As outer tiles are destroyed, newly exposed inner tiles become visible automatically (no extra work needed — `IsRevealed()` re-checks on each draw).

---

## Per-Tile Reveal State

```cpp
// In Tile struct (tile.h):
bool revealed = false;   // set true by drone scan

// In Asteroid:
bool IsTileVisible(int col, int row) const {
    if (grid.cells[row][col].revealed) return true;
    // Check 4 neighbors
    int dc[] = {1,-1,0,0};
    int dr[] = {0,0,1,-1};
    for (int i = 0; i < 4; i++) {
        int nc = col + dc[i], nr = row + dr[i];
        if (nc < 0 || nc >= grid.cols || nr < 0 || nr >= grid.rows)
            return true;  // edge tile = surface
        if (grid.cells[nr][nc].material == Material::None)
            return true;  // adjacent to empty = surface
    }
    return false;
}
```

Draw call uses this: if `!IsTileVisible(c, r)`, draw tile as `DARKGRAY` with no material info.

---

## Intel Drone

```cpp
class IntelDrone {
public:
    Vector2  pos;
    Vector2  vel;           // ~200 px/s toward target
    bool     landed = false;
    bool     alive  = true;
    Asteroid* attached_to = nullptr;

    void Update(float dt, std::vector<Asteroid>& asteroids);
    void Draw();

private:
    void Land(Asteroid* ast, Vector2 surface_pos);
    void Scan(Asteroid* ast);  // reveals tiles within SCAN_RADIUS
};

static constexpr float SCAN_RADIUS = 150.f;
```

### Landing & Scan

```cpp
void IntelDrone::Land(Asteroid* ast, Vector2 surface_pos) {
    landed = true;
    attached_to = ast;
    vel = {0, 0};
    Scan(ast);
}

void IntelDrone::Scan(Asteroid* ast) {
    // World position of scan center = pos
    // Iterate all tiles in asteroid, reveal those within SCAN_RADIUS
    Vector2 center = ast->GridCenter();
    for (int r = 0; r < ast->grid.rows; r++) {
        for (int c = 0; c < ast->grid.cols; c++) {
            Vector2 tile_world = ast->TileWorldPos(c, r);
            if (Vector2Distance(tile_world, pos) <= SCAN_RADIUS) {
                ast->grid.cells[r][c].revealed = true;
            }
        }
    }
    // Build scan report for HUD
    BuildScanReport(ast);
}
```

### Scan Report

```cpp
struct ScanReport {
    std::unordered_map<Material, int> tile_counts;
    float display_timer = 5.f;   // show in HUD for 5 seconds
};
```

HUD renders this as a text popup in the top-right corner while `display_timer > 0`.

---

## Drawing Fog

Hidden tiles are drawn in a darkened flat color (no material info visible):

```cpp
Color fog_color = { 30, 30, 35, 255 };   // near-black with slight blue

if (!IsTileVisible(c, r)) {
    DrawRectangleV(world_pos, {TILE_SIZE, TILE_SIZE}, fog_color);
} else {
    DrawRectangleV(world_pos, {TILE_SIZE, TILE_SIZE}, MaterialColor(t.material));
}
```

No separate render texture needed — drawn inline with the tile loop.

---

## Minimap

Small `RenderTexture2D` (200×200 px) drawn in top-right HUD corner.

Update at 10 Hz (not every frame):
- Draw each asteroid's tiles as 1-pixel dots (color = material if revealed, dark grey if not)
- Draw ship as a 3-pixel white dot
- Draw drone positions as cyan dots
- Draw free chunks as colored dots

Scale: minimap covers the full 4000×4000 world, so 1 minimap px = 20 world px.
