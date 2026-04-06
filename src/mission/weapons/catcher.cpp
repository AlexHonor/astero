#include "catcher.h"
#include "mission/chunk.h"

void CatcherProjectile::Draw() const {
    if (!alive) return;
    DrawCircleLines((int)pos.x, (int)pos.y, 6.f, SKYBLUE);
    // Tether lines drawn from ship (ship_pos updated each frame)
}

void CatcherProjectile::Update(float dt, std::vector<Asteroid>& asteroids,
                                 std::vector<Chunk>& chunks) {
    (void)asteroids;
    if (!alive) return;

    lifetime -= dt;
    if (lifetime <= 0.f) { Kill(); return; }

    pos.x += vel.x * dt;
    pos.y += vel.y * dt;

    // Detect nearby chunks and tether them
    for (int i = 0; i < (int)chunks.size(); i++) {
        if (!chunks[i].alive) continue;
        if ((int)tethered.size() >= max_tethers) break;
        bool already = false;
        for (int t : tethered) if (t == i) { already = true; break; }
        if (already) continue;

        if (Vector2Distance(chunks[i].pos, pos) < detect_radius)
            tethered.push_back(i);
    }

    // Pull tethered chunks toward ship
    for (int idx : tethered) {
        if (idx >= (int)chunks.size() || !chunks[idx].alive) continue;
        Vector2 dir = Vector2Normalize(Vector2Subtract(ship_pos, chunks[idx].pos));
        chunks[idx].vel = Vector2Scale(dir, tether_speed);
    }
}
