#include "flare.h"
#include "mission/asteroid.h"

void FlareProjectile::Draw() const {
    if (!alive) return;
    Color c = landed ? Color{255, 180, 60, 220} : ORANGE;
    DrawCircle((int)pos.x, (int)pos.y, 4.f, c);
    if (landed) {
        // Small pulsing glow
        float pulse = (sinf(GetTime() * 6.f) * 0.5f + 0.5f);
        DrawCircleLines((int)pos.x, (int)pos.y, 6.f + pulse * 3.f,
                        {255, 200, 80, 100});
    }
}

void FlareProjectile::Update(float dt, std::vector<Asteroid>& asteroids,
                               std::vector<Chunk>& chunks) {
    (void)chunks;
    if (!alive) return;

    if (!landed) {
        lifetime -= dt;
        if (lifetime <= 0.f) {
            // Auto-land wherever it is
            landed = true;
            vel    = {0, 0};
            light_life = 15.f;
        }

        pos.x += vel.x * dt;
        pos.y += vel.y * dt;

        // Check surface landing
        for (auto& ast : asteroids) {
            for (int r = 0; r < ast.rows; r++) {
                for (int c = 0; c < ast.cols; c++) {
                    if (ast.cells[r][c].material == TileMat::None) continue;
                    Vector2 tp = ast.TileWorldPos(c, r);
                    if (Vector2Distance(pos, tp) < TILE_SIZE * 0.8f) {
                        landed = true;
                        vel    = {0, 0};
                        light_life = 15.f;
                        goto done;
                    }
                }
            }
        }
        done:;
    } else {
        light_life -= dt;
        if (light_life <= 0.f) Kill();
    }
}
