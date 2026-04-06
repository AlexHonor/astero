#include "projectile.h"
#include "asteroid.h"
#include "chunk.h"

void Projectile::Update(float dt, std::vector<Asteroid>& asteroids,
                        std::vector<Chunk>& chunks) {
    (void)chunks;
    if (!alive) return;

    lifetime -= dt;
    if (lifetime <= 0.f) { Kill(); return; }

    prev_pos = pos;
    pos.x += vel.x * dt;
    pos.y += vel.y * dt;

    CheckTileHit(asteroids);
}

void Projectile::Draw() const {
    if (!alive) return;
    DrawLine((int)pos.x, (int)pos.y, (int)(pos.x - 0.03 * vel.x), (int)(pos.y - 0.03 * vel.y), YELLOW);
    DrawCircle((int)pos.x, (int)pos.y, 3.f, YELLOW);
}

// Slab-method ray vs AABB. Ray is prev_pos + t*(pos-prev_pos), t in [0,1].
// Returns t of first entry, or -1 if no hit.
static float RayAABB(Vector2 origin, Vector2 dir,
                     Vector2 box_min, Vector2 box_max) {
    float tmin = 0.f, tmax = 1.f;
    float os[2] = {origin.x, origin.y};
    float ds[2] = {dir.x,    dir.y};
    float lo[2] = {box_min.x, box_min.y};
    float hi[2] = {box_max.x, box_max.y};
    for (int i = 0; i < 2; i++) {
        if (fabsf(ds[i]) < 1e-6f) {
            if (os[i] < lo[i] || os[i] > hi[i]) return -1.f;
        } else {
            float t1 = (lo[i] - os[i]) / ds[i];
            float t2 = (hi[i] - os[i]) / ds[i];
            if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
            tmin = fmaxf(tmin, t1);
            tmax = fminf(tmax, t2);
            if (tmin > tmax) return -1.f;
        }
    }
    return tmin;
}

bool Projectile::CheckTileHit(std::vector<Asteroid>& asteroids) {
    Vector2 dir = {pos.x - prev_pos.x, pos.y - prev_pos.y};
    const float half = TILE_SIZE * 0.5f;

    float best_t  = 2.f;
    Asteroid* hit_ast = nullptr;
    int hit_c = -1, hit_r = -1;

    for (auto& ast : asteroids) {
        for (int r = 0; r < ast.rows; r++) {
            for (int c = 0; c < ast.cols; c++) {
                if (ast.cells[r][c].material == TileMat::None) continue;
                Vector2 tp = ast.TileWorldPos(c, r);
                float t = RayAABB(prev_pos, dir,
                                  {tp.x - half, tp.y - half},
                                  {tp.x + half, tp.y + half});
                if (t >= 0.f && t < best_t) {
                    best_t  = t;
                    hit_ast = &ast;
                    hit_c   = c;
                    hit_r   = r;
                }
            }
        }
    }

    if (!hit_ast) return false;

    // Snap bullet to hit point
    pos.x = prev_pos.x + dir.x * best_t;
    pos.y = prev_pos.y + dir.y * best_t;

    Tile& t = hit_ast->cells[hit_r][hit_c];
    if (penetration < t.armor) {
        float roll = (float)rand() / RAND_MAX;
        if (roll < t.ricochet) {
            Vector2 tp = hit_ast->TileWorldPos(hit_c, hit_r);
            Vector2 normal = Vector2Normalize(Vector2Subtract(pos, tp));
            float dot = Vector2DotProduct(vel, normal);
            vel.x = (vel.x - 2*dot*normal.x) * 0.7f;
            vel.y = (vel.y - 2*dot*normal.y) * 0.7f;
        } else {
            Kill();
        }
    } else {
        hit_ast->DamageTile(hit_c, hit_r, damage, penetration);
        Kill();
    }
    return true;
}
