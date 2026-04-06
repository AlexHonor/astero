#include "minimap.h"
#include <cmath>

void Minimap::Init() {
    tex = LoadRenderTexture(MAP_SIZE, MAP_SIZE);
}

void Minimap::Shutdown() {
    UnloadRenderTexture(tex);
}

void Minimap::Update(const std::vector<Asteroid>& asteroids,
                     const std::vector<Chunk>& chunks,
                     const Ship& ship) {
    update_timer += GetFrameTime();
    if (update_timer < 1.f / UPDATE_HZ) return;
    update_timer = 0.f;

    float scale = MAP_SIZE / WORLD_SIZE;

    BeginTextureMode(tex);
    ClearBackground(Color{10, 10, 20, 255});

    // Draw asteroid tiles as 1px dots
    for (auto& ast : asteroids) {
        for (int r = 0; r < ast.rows; r++) {
            for (int c = 0; c < ast.cols; c++) {
                TileMat m = ast.cells[r][c].material;
                if (m == TileMat::None) continue;
                Vector2 wp = ast.TileWorldPos(c, r);
                int mx = (int)(wp.x * scale);
                int my = (int)(wp.y * scale);
                if (mx < 0 || mx >= MAP_SIZE || my < 0 || my >= MAP_SIZE) continue;

                Color dot;
                if (!ast.IsTileVisible(c, r) && !ast.cells[r][c].revealed)
                    dot = Color{50, 50, 55, 255};
                else
                    dot = MaterialColor(m);
                DrawPixel(mx, my, dot);
            }
        }
    }

    // Chunks
    for (auto& ch : chunks) {
        if (!ch.alive) continue;
        int mx = (int)(ch.pos.x * scale);
        int my = (int)(ch.pos.y * scale);
        if (mx < 1 || mx >= MAP_SIZE-1 || my < 1 || my >= MAP_SIZE-1) continue;
        DrawPixel(mx,   my,   MaterialColor(ch.material));
        DrawPixel(mx+1, my,   MaterialColor(ch.material));
    }

    // Ship
    int sx = (int)(ship.pos.x * scale);
    int sy = (int)(ship.pos.y * scale);
    sx = sx < 1 ? 1 : (sx >= MAP_SIZE-1 ? MAP_SIZE-2 : sx);
    sy = sy < 1 ? 1 : (sy >= MAP_SIZE-1 ? MAP_SIZE-2 : sy);
    DrawPixel(sx-1, sy,   WHITE);
    DrawPixel(sx+1, sy,   WHITE);
    DrawPixel(sx,   sy-1, WHITE);
    DrawPixel(sx,   sy+1, WHITE);
    DrawPixel(sx,   sy,   WHITE);

    EndTextureMode();
}

void Minimap::Draw(int screen_x, int screen_y) const {
    // Draw border
    DrawRectangle(screen_x - 2, screen_y - 2, MAP_SIZE + 4, MAP_SIZE + 4,
                  Color{40, 40, 50, 200});
    DrawRectangleLines(screen_x - 2, screen_y - 2, MAP_SIZE + 4, MAP_SIZE + 4,
                       Color{100, 100, 120, 255});

    // Flip Y — raylib render textures are upside down
    DrawTextureRec(tex.texture,
        {0, 0, (float)MAP_SIZE, -(float)MAP_SIZE},
        {(float)screen_x, (float)screen_y}, WHITE);
}
