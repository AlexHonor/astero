#include "dust_sim.h"
#include <cmath>
#include <algorithm>

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

void DustSim::AddExplosionDust(Vector2 center, float radius, TileMat mat) {
    int cell_r = (int)(radius / CELL_SIZE) + 1;
    float cx = center.x / CELL_SIZE;
    float cy = center.y / CELL_SIZE;
    for (int dr = -cell_r; dr <= cell_r; dr++) {
        for (int dc = -cell_r; dc <= cell_r; dc++) {
            if (dc*dc + dr*dr > cell_r * cell_r) continue;
            float dist = sqrtf((float)(dc*dc + dr*dr));
            if (dist < 0.01f) continue;
            Vector2 outward = {(float)dc / dist * 2.f, (float)dr / dist * 2.f};
            AddDust({(cx + dc) * CELL_SIZE, (cy + dr) * CELL_SIZE}, mat,
                    {outward.x * 200.f, outward.y * 200.f});
        }
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
        c.density *= 0.97f * extra_decay;

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
