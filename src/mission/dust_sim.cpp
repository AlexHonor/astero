#include "dust_sim.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>

Color DustSim::MatDustColor(TileMat m) {
    switch (m) {
        case TileMat::Rock:     return {80,  80,  85,  255};
        case TileMat::Iron:     return {120, 70,  40,  255};
        case TileMat::Copper:   return {70,  130, 80,  255};
        case TileMat::Silicon:  return {160, 160, 190, 255};
        case TileMat::Titanium: return {60,  90,  130, 255};
        case TileMat::Gold:     return {200, 160, 0,   255};
        default:                return {80,  80,  85,  255};
    }
}

void DustSim::Init() {
    grid.resize((size_t)GRID_W * GRID_H);
}

void DustSim::Shutdown() {
    grid.clear();
    active.clear();
}

void DustSim::AddDust(Vector2 world_pos, TileMat mat, Vector2 impulse) {
    int col = (int)(world_pos.x / CELL_SIZE);
    int row = (int)(world_pos.y / CELL_SIZE);
    if (!InBounds(col, row)) return;

    int idx = row * GRID_W + col;
    DustCell& c = grid[idx];
    c.density = fminf(1.f, c.density + 0.8f);
    c.color   = MatDustColor(mat);
    c.vx      = impulse.x * 0.01f;
    c.vy      = impulse.y * 0.01f;
    active.insert(idx);
}

static float frand() { return (float)(rand() % 1000) / 1000.f; }  // [0,1)

void DustSim::AddExplosionDust(Vector2 center, float radius, TileMat mat) {
    // Primary blast ring: dense smoke spread across a larger area
    int cell_r = (int)(radius / CELL_SIZE) + 3;
    float cx = center.x / CELL_SIZE;
    float cy = center.y / CELL_SIZE;
    for (int dr = -cell_r; dr <= cell_r; dr++) {
        for (int dc = -cell_r; dc <= cell_r; dc++) {
            float dist2 = (float)(dc*dc + dr*dr);
            if (dist2 > (float)(cell_r * cell_r)) continue;
            float dist = sqrtf(dist2);

            // Random jitter on position to break up the circle
            float jitter_x = (frand() - 0.5f) * 1.6f;
            float jitter_y = (frand() - 0.5f) * 1.6f;
            float wx = (cx + dc + jitter_x) * CELL_SIZE;
            float wy = (cy + dr + jitter_y) * CELL_SIZE;

            // Outward impulse, stronger at center, random magnitude variation
            Vector2 outward = {0.f, 0.f};
            if (dist > 0.01f) {
                outward = {(float)dc / dist, (float)dr / dist};
            } else {
                float angle = frand() * 6.2832f;
                outward = {cosf(angle), sinf(angle)};
            }
            float speed = (200.f + frand() * 300.f) * (1.f - dist / (cell_r + 1));
            Vector2 impulse = {outward.x * speed, outward.y * speed};

            int col = (int)(wx / CELL_SIZE);
            int row = (int)(wy / CELL_SIZE);
            if (!InBounds(col, row)) continue;
            int idx = row * GRID_W + col;
            DustCell& c = grid[idx];
            // Add dense smoke with random density variation
            float add = 0.7f + frand() * 0.3f;
            c.density = fminf(1.f, c.density + add);
            c.color   = MatDustColor(mat);
            c.vx      = impulse.x * 0.01f + (frand() - 0.5f) * 0.3f;
            c.vy      = impulse.y * 0.01f + (frand() - 0.5f) * 0.3f;
            active.insert(idx);
        }
    }

    // Secondary lingering smoke cloud: slower, denser, fills interior
    int smoke_r = cell_r + 2;
    for (int i = 0; i < 120; i++) {
        float angle = frand() * 6.2832f;
        float r     = frand() * smoke_r;
        float wx    = (cx + cosf(angle) * r) * CELL_SIZE;
        float wy    = (cy + sinf(angle) * r) * CELL_SIZE;
        int col = (int)(wx / CELL_SIZE);
        int row = (int)(wy / CELL_SIZE);
        if (!InBounds(col, row)) continue;
        int idx = row * GRID_W + col;
        DustCell& c = grid[idx];
        c.density = fminf(1.f, c.density + 0.5f + frand() * 0.5f);
        c.color   = MatDustColor(mat);
        // Slow outward drift with upward bias
        float slow = frand() * 0.4f;
        c.vx = cosf(angle) * slow + (frand() - 0.5f) * 0.2f;
        c.vy = sinf(angle) * slow - frand() * 0.3f;  // slight upward
        active.insert(idx);
    }
}

void DustSim::Step() {
    std::unordered_set<int> next;
    next.reserve(active.size() * 2);

    // Cap: if too many active cells, force-decay
    float extra_decay = active.size() > MAX_ACTIVE ? 0.94f : 1.f;

    for (int idx : active) {
        int col = idx % GRID_W;
        int row = idx / GRID_W;
        DustCell& c = grid[idx];

        // 1. Decay
        c.density *= 0.985f * extra_decay;

        // 2. Drift
        int drift_col = col + (int)roundf(c.vx);
        int drift_row = row + (int)roundf(c.vy);
        if (InBounds(drift_col, drift_row) && (drift_col != col || drift_row != row)) {
            float deposit = c.density * 0.15f;
            c.density -= deposit;
            int nidx = drift_row * GRID_W + drift_col;
            DustCell& nc = grid[nidx];
            nc.density = fminf(1.f, nc.density + deposit);
            nc.color   = c.color;
            nc.vx      = c.vx * 0.95f;
            nc.vy      = c.vy * 0.95f;
            next.insert(nidx);
        }

        // 3. Diffuse (4 neighbors)
        const int dc[] = {1,-1,0,0};
        const int dr[] = {0,0,1,-1};
        float spread = c.density * 0.02f;
        c.density -= spread * 4.f;
        for (int i = 0; i < 4; i++) {
            int nc2 = col + dc[i], nr2 = row + dr[i];
            if (!InBounds(nc2, nr2)) continue;
            int nidx2 = nr2 * GRID_W + nc2;
            grid[nidx2].density = fminf(1.f, grid[nidx2].density + spread);
            grid[nidx2].color   = c.color;
            next.insert(nidx2);
        }

        c.vx *= 0.9f;
        c.vy *= 0.9f;

        if (c.density > 0.01f) next.insert(idx);
        else c.density = 0.f;
    }

    active = std::move(next);
}

void DustSim::Update(float dt) {
    sim_timer += dt;
    if (sim_timer >= 1.f / SIM_HZ) {
        sim_timer = 0.f;
        Step();
    }
}

float DustSim::DensityAt(Vector2 world_pos) const {
    int col = (int)(world_pos.x / CELL_SIZE);
    int row = (int)(world_pos.y / CELL_SIZE);
    if (!InBounds(col, row)) return 0.f;
    return grid[row * GRID_W + col].density;
}

void DustSim::Draw(Camera2D& cam) {
    if (active.empty()) return;

    BeginMode2D(cam);
    BeginBlendMode(BLEND_ALPHA);

    for (int idx : active) {
        int col = idx % GRID_W;
        int row = idx / GRID_W;
        const DustCell& c = grid[idx];
        if (c.density < 0.01f) continue;

        Color dc = c.color;
        dc.a = (unsigned char)(c.density * 180.f);
        DrawRectangle(col * CELL_SIZE, row * CELL_SIZE, CELL_SIZE, CELL_SIZE, dc);
    }

    EndBlendMode();
    EndMode2D();
}
