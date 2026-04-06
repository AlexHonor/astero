#include "explosive.h"
#include "mission/asteroid.h"
#include <cmath>

// --- Explosion ---

void Explosion::Apply(std::vector<Asteroid>& asteroids) {
    for (auto& ast : asteroids)
        ast.ExplosionDamage(pos, radius, damage, pen);
}

void Explosion::Draw() const {
    float frac = timer / 0.4f;
    float r    = radius * (1.f - frac * 0.3f);
    Color c    = {255, (unsigned char)(140 * frac), 20, (unsigned char)(200 * frac)};
    DrawCircleLines((int)pos.x, (int)pos.y, r, c);
    DrawCircleLines((int)pos.x, (int)pos.y, r * 0.5f,
                    {255, 255, 100, (unsigned char)(120 * frac)});
}

// --- ExplosiveShell ---

void ExplosiveShell::Draw() const {
    if (!alive) return;
    DrawCircle((int)pos.x, (int)pos.y, 5.f, ORANGE);
}

void ExplosiveShell::Update(float dt, std::vector<Asteroid>& asteroids,
                             std::vector<Chunk>& chunks) {
    (void)chunks;
    if (!alive) return;
    lifetime -= dt;
    if (lifetime <= 0.f) { Kill(); return; }

    pos.x += vel.x * dt;
    pos.y += vel.y * dt;

    if (CheckTileHit(asteroids)) {
        // Explode
        if (explosions) explosions->push_back({pos, exp_radius, exp_damage, penetration});
        // ExplosionDamage already applied through CheckTileHit's DamageTile,
        // but we want the area explosion too
        for (auto& ast : asteroids)
            ast.ExplosionDamage(pos, exp_radius, exp_damage, penetration);
        Kill();
    }
}

// --- ShrapnelBullet ---

void ShrapnelBullet::Draw() const {
    if (!alive) return;
    DrawCircle((int)pos.x, (int)pos.y, 4.f, ORANGE);
}

void ShrapnelBullet::Update(float dt, std::vector<Asteroid>& asteroids,
                             std::vector<Chunk>& chunks) {
    (void)chunks;
    if (!alive) return;
    lifetime     -= dt;
    fuse_elapsed += dt;

    if (lifetime <= 0.f) { Kill(); return; }

    pos.x += vel.x * dt;
    pos.y += vel.y * dt;

    if (CheckTileHit(asteroids)) { Kill(); return; }

    if (fuse_elapsed >= fuse_time && spawn_fragment) {
        float base_angle = atan2f(vel.y, vel.x);
        float spread     = 3.14159f / 6.f;  // ±30 degrees
        for (int i = 0; i < frag_count; i++) {
            float a = base_angle + spread * (i / (float)(frag_count - 1) - 0.5f) * 2.f;
            float spd = Vector2Length(vel) * 0.9f;
            spawn_fragment(pos, {cosf(a) * spd, sinf(a) * spd});
        }
        Kill();
    }
}

// --- ClusterBomb ---

void ClusterBomb::Draw() const {
    if (!alive) return;
    DrawCircle((int)pos.x, (int)pos.y, 6.f, RED);
}

void ClusterBomb::Update(float dt, std::vector<Asteroid>& asteroids,
                          std::vector<Chunk>& chunks) {
    (void)chunks;
    if (!alive) return;
    lifetime    -= dt;
    arm_elapsed += dt;

    if (lifetime <= 0.f) { Kill(); return; }

    pos.x += vel.x * dt;
    pos.y += vel.y * dt;

    if (CheckTileHit(asteroids)) { Kill(); return; }

    if (arm_elapsed >= arm_time && spawn_sub) {
        float base_angle = atan2f(vel.y, vel.x);
        for (int i = 0; i < sub_count; i++) {
            float a = base_angle + (3.14159f * 2.f / sub_count) * i;
            float spd = 200.f;
            spawn_sub(pos, {cosf(a)*spd, sinf(a)*spd}, sub_exp_r, sub_exp_dmg);
        }
        Kill();
    }
}

// --- TimedMine ---

void TimedMine::Draw() const {
    if (!alive) return;
    bool blink = (int)(fuse_elapsed * 4) % 2 == 0;
    Color c = blink ? RED : ORANGE;
    DrawCircle((int)pos.x, (int)pos.y, 5.f, c);
    if (embedded) {
        float pct = fuse_elapsed / fuse_time;
        DrawCircleLines((int)pos.x, (int)pos.y, 5.f + pct * 3.f, WHITE);
    }
}

void TimedMine::Update(float dt, std::vector<Asteroid>& asteroids,
                        std::vector<Chunk>& chunks) {
    (void)chunks;
    if (!alive) return;

    if (!embedded) {
        lifetime -= dt;
        if (lifetime <= 0.f) { Kill(); return; }
        pos.x += vel.x * dt;
        pos.y += vel.y * dt;

        // Check for surface hit to embed
        for (auto& ast : asteroids) {
            for (int r = 0; r < ast.rows; r++) {
                for (int c = 0; c < ast.cols; c++) {
                    if (ast.cells[r][c].material == TileMat::None) continue;
                    Vector2 tp = ast.TileWorldPos(c, r);
                    if (Vector2Distance(pos, tp) < TILE_SIZE) {
                        embedded = true;
                        vel = {0, 0};
                        // Compute attachment in asteroid local space
                        attached_ast_id = ast.id;
                        float dx =  pos.x - ast.center.x;
                        float dy =  pos.y - ast.center.y;
                        float cs = cosf(-ast.rotation);
                        float sn = sinf(-ast.rotation);
                        ast_local_offset = {dx * cs - dy * sn, dx * sn + dy * cs};
                        goto done_embed;
                    }
                }
            }
        }
        done_embed:;
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
        fuse_elapsed += dt;
        if (fuse_elapsed >= fuse_time || detonate_now) {
            if (explosions)
                explosions->push_back({pos, exp_radius, exp_damage, penetration});
            for (auto& ast : asteroids)
                ast.ExplosionDamage(pos, exp_radius, exp_damage, penetration);
            Kill();
        }
    }
}
