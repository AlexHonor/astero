#include "flare.h"
#include "mission/asteroid.h"
#include <cmath>

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
                        // Compute attachment in asteroid local space
                        attached_ast_id = ast.id;
                        float dx =  pos.x - ast.center.x;
                        float dy =  pos.y - ast.center.y;
                        float cs = cosf(-ast.rotation);
                        float sn = sinf(-ast.rotation);
                        ast_local_offset = {dx * cs - dy * sn, dx * sn + dy * cs};
                        goto done;
                    }
                }
            }
        }
        done:;
    } else {
        // Track the host asteroid — update world position to follow rotation/drift
        if (attached_ast_id >= 0) {
            for (auto& ast : asteroids) {
                if (ast.id == attached_ast_id) {
                    float cs = cosf(ast.rotation);
                    float sn = sinf(ast.rotation);
                    pos.x = ast.center.x + ast_local_offset.x * cs - ast_local_offset.y * sn;
                    pos.y = ast.center.y + ast_local_offset.x * sn + ast_local_offset.y * cs;
                    break;
                }
            }
        }
        // Flares shine forever — no kill timer
    }
}
