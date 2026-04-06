#include "intel_drone.h"
#include "mission/asteroid.h"

void IntelDrone::Draw() const {
    if (!alive) return;
    // Diamond shape
    float s = landed ? 4.f : 6.f;
    Color c = landed ? Color{100, 200, 255, 255} : SKYBLUE;
    DrawPoly(pos, 4, s, 45.f, c);
    DrawPolyLines(pos, 4, s + 1.f, 45.f, WHITE);
}

void IntelDrone::Update(float dt, std::vector<Asteroid>& asteroids,
                         std::vector<Chunk>& chunks) {
    (void)chunks;
    if (!alive) return;

    if (!landed) {
        lifetime -= dt;
        if (lifetime <= 0.f) { Kill(); return; }   // drifted out, wasted

        pos.x += vel.x * dt;
        pos.y += vel.y * dt;

        // Check surface landing
        for (auto& ast : asteroids) {
            for (int r = 0; r < ast.rows; r++) {
                for (int c = 0; c < ast.cols; c++) {
                    if (ast.cells[r][c].material == TileMat::None) continue;
                    if (ast.IsTileVisible(c, r)) {
                        Vector2 tp = ast.TileWorldPos(c, r);
                        if (Vector2Distance(pos, tp) < TILE_SIZE * 1.5f) {
                            landed = true;
                            vel    = {0, 0};
                            DoScan(asteroids);
                            return;
                        }
                    }
                }
            }
        }
    }
    // Landed — stay forever until mission ends
}

void IntelDrone::DoScan(std::vector<Asteroid>& asteroids) {
    if (!report) return;
    report->tile_counts.clear();
    report->has_data     = true;
    report->display_timer = 5.f;

    for (auto& ast : asteroids) {
        for (int r = 0; r < ast.rows; r++) {
            for (int c = 0; c < ast.cols; c++) {
                TileMat m = ast.cells[r][c].material;
                if (m == TileMat::None) continue;
                Vector2 tp = ast.TileWorldPos(c, r);
                if (Vector2Distance(tp, pos) <= scan_radius) {
                    ast.cells[r][c].revealed = true;
                    report->tile_counts[m]++;
                }
            }
        }
    }
}
