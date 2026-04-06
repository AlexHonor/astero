#include "projectile.h"
#include "asteroid.h"
#include "chunk.h"

void Projectile::Update(float dt, std::vector<Asteroid>& asteroids,
                        std::vector<Chunk>& chunks) {
    (void)chunks;
    if (!alive) return;

    lifetime -= dt;
    if (lifetime <= 0.f) { Kill(); return; }

    pos.x += vel.x * dt;
    pos.y += vel.y * dt;

    CheckTileHit(asteroids);
}

void Projectile::Draw() const {
    if (!alive) return;
    DrawLine((int)pos.x, (int)pos.y, (int)(pos.x - 0.03 * vel.x), (int)(pos.y - 0.03 * vel.y), YELLOW);
    DrawCircle((int)pos.x, (int)pos.y, 3.f, YELLOW);
}

bool Projectile::CheckTileHit(std::vector<Asteroid>& asteroids) {
    for (auto& ast : asteroids) {
        for (int r = 0; r < ast.rows; r++) {
            for (int c = 0; c < ast.cols; c++) {
                if (ast.cells[r][c].material == TileMat::None) continue;
                Vector2 tp = ast.TileWorldPos(c, r);
                float half = TILE_SIZE * 0.5f;
                if (fabsf(pos.x - tp.x) < half && fabsf(pos.y - tp.y) < half) {
                    Tile& t = ast.cells[r][c];
                    if (penetration < t.armor) {
                        float roll = (float)rand() / RAND_MAX;
                        if (roll < t.ricochet) {
                            // Reflect velocity
                            Vector2 normal = Vector2Normalize(Vector2Subtract(pos, tp));
                            float dot = Vector2DotProduct(vel, normal);
                            vel.x = (vel.x - 2*dot*normal.x) * 0.7f;
                            vel.y = (vel.y - 2*dot*normal.y) * 0.7f;
                        } else {
                            Kill();
                        }
                    } else {
                        ast.DamageTile(c, r, damage, penetration);
                        Kill();
                    }
                    return true;
                }
            }
        }
    }
    return false;
}
