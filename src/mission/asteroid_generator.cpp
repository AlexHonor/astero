#include "asteroid_generator.h"
#include "raymath.h"
#include <cmath>
#include <cstring>
#include <queue>
#include <utility>

// --- Simple deterministic noise ---

static uint32_t Hash(uint32_t x) {
    x ^= x >> 16; x *= 0x45d9f3b;
    x ^= x >> 16; x *= 0x45d9f3b;
    x ^= x >> 16;
    return x;
}

static float Randf01(uint32_t& state) {
    state = Hash(state + 1);
    return (state & 0xFFFFFF) / (float)0x1000000;
}

float AsteroidGenerator::ValueNoise(float x, uint32_t seed) {
    int ix = (int)floorf(x);
    float frac = x - ix;
    frac = frac * frac * (3 - 2 * frac);  // smoothstep
    uint32_t s0 = Hash((uint32_t)ix ^ seed);
    uint32_t s1 = Hash((uint32_t)(ix+1) ^ seed);
    float v0 = (s0 & 0xFFFFFF) / (float)0x1000000;
    float v1 = (s1 & 0xFFFFFF) / (float)0x1000000;
    return v0 + (v1 - v0) * frac;
}

// Returns noisy radius at angle theta (radians) relative to base_r
float AsteroidGenerator::PolarRadius(float theta, float base_r, uint32_t seed) {
    float n1 = ValueNoise(theta * 3.f / (2.f * 3.14159f) * 8.f, seed);
    float n2 = ValueNoise(theta * 3.f / (2.f * 3.14159f) * 16.f, seed ^ 0xDEAD);
    return base_r * (0.65f + 0.25f * n1 + 0.10f * n2);
}

void AsteroidGenerator::BuildAsteroid(Asteroid& ast, int w, int h, uint32_t seed, float deep_bias) {
    float cx = w * 0.5f;
    float cy = h * 0.5f;
    float base_r = fminf(cx, cy) * 0.85f;

    // Step 1: polar mask
    std::vector<std::vector<bool>> solid(h, std::vector<bool>(w, false));
    for (int r = 0; r < h; r++) {
        for (int c = 0; c < w; c++) {
            float dx = c + 0.5f - cx;
            float dy = r + 0.5f - cy;
            float dist = sqrtf(dx*dx + dy*dy);
            float theta = atan2f(dy, dx) + 3.14159f;
            float noisy_r = PolarRadius(theta, base_r, seed);
            solid[r][c] = (dist < noisy_r);
        }
    }

    // Step 2: compute depth via BFS distance field from surface
    std::vector<std::vector<float>> depth(h, std::vector<float>(w, 0.f));
    {
        std::queue<std::pair<int,int>> q;
        std::vector<std::vector<int>> dist_grid(h, std::vector<int>(w, -1));
        const int dc[] = {1,-1,0,0};
        const int dr[] = {0,0,1,-1};

        // Seed BFS from all empty or border cells adjacent to solid
        for (int r = 0; r < h; r++)
            for (int c = 0; c < w; c++)
                if (solid[r][c]) {
                    bool is_surface = false;
                    for (int i = 0; i < 4 && !is_surface; i++) {
                        int nc = c + dc[i], nr = r + dr[i];
                        if (nc < 0 || nc >= w || nr < 0 || nr >= h || !solid[nr][nc])
                            is_surface = true;
                    }
                    if (is_surface) { dist_grid[r][c] = 0; q.push({c,r}); }
                }

        while (!q.empty()) {
            auto [qc, qr] = q.front(); q.pop();
            for (int i = 0; i < 4; i++) {
                int nc = qc + dc[i], nr = qr + dr[i];
                if (nc < 0 || nc >= w || nr < 0 || nr >= h) continue;
                if (!solid[nr][nc]) continue;
                if (dist_grid[nr][nc] != -1) continue;
                dist_grid[nr][nc] = dist_grid[qr][qc] + 1;
                q.push({nc, nr});
            }
        }

        // Normalize
        int max_d = 1;
        for (int r = 0; r < h; r++)
            for (int c = 0; c < w; c++)
                if (dist_grid[r][c] > max_d) max_d = dist_grid[r][c];

        for (int r = 0; r < h; r++)
            for (int c = 0; c < w; c++)
                if (dist_grid[r][c] >= 0)
                    depth[r][c] = dist_grid[r][c] / (float)max_d;
    }

    // Step 3: assign materials by depth + noise veins
    uint32_t mat_seed = seed ^ 0xABCD1234;
    for (int r = 0; r < h; r++) {
        for (int c = 0; c < w; c++) {
            if (!solid[r][c]) continue;

            float d = depth[r][c];
            // Vein noise for rich pockets
            float vein = ValueNoise(c * 0.3f + r * 0.17f, mat_seed);

            TileMat mat;
            if (d < 0.20f)
                mat = TileMat::Rock;
            else if (d < 0.45f)
                mat = (vein > 0.65f + deep_bias) ? TileMat::Copper : TileMat::Iron;
            else if (d < 0.70f)
                mat = (vein > 0.55f + deep_bias) ? TileMat::Silicon
                    : (vein > 0.75f + deep_bias) ? TileMat::Titanium
                    : TileMat::Copper;
            else
                mat = (vein > 0.60f + deep_bias) ? TileMat::Gold : TileMat::Titanium;

            ast.cells[r][c] = MakeTile(mat);
        }
    }

    // Step 4: punch cavities
    uint32_t cav_seed = seed ^ 0xFEED;
    uint32_t cav_state = cav_seed;
    int num_cavities = (int)(w * h * 0.04f);
    for (int i = 0; i < num_cavities; i++) {
        int cc = (int)(Randf01(cav_state) * w);
        int cr = (int)(Randf01(cav_state) * h);
        int rad = 1 + (int)(Randf01(cav_state) * 3);
        if (depth[cr][cc] < 0.3f) continue;  // only in interior
        for (int dr2 = -rad; dr2 <= rad; dr2++)
            for (int dc2 = -rad; dc2 <= rad; dc2++) {
                int nc = cc+dc2, nr = cr+dr2;
                if (nc<0||nc>=w||nr<0||nr>=h) continue;
                if (dc2*dc2+dr2*dr2 <= rad*rad)
                    ast.cells[nr][nc].material = TileMat::None;
            }
    }
}

void AsteroidGenerator::Generate(std::vector<Asteroid>& field, FieldConfig cfg, uint32_t seed) {
    field.clear();
    uint32_t state = seed;
    std::vector<Vector2> positions;

    int attempts = 0;
    while ((int)positions.size() < cfg.num_asteroids && attempts < 500) {
        attempts++;
        float angle = Randf01(state) * 6.28318f;
        float dist  = Randf01(state) * cfg.field_radius;
        Vector2 pos = {
            2000.f + cosf(angle) * dist,
            2000.f + sinf(angle) * dist
        };

        bool ok = true;
        for (auto& p : positions)
            if (Vector2Distance(p, pos) < cfg.min_dist) { ok = false; break; }

        if (!ok) continue;
        positions.push_back(pos);

        int w = cfg.min_size + (int)(Randf01(state) * (cfg.max_size - cfg.min_size));
        int h = cfg.min_size + (int)(Randf01(state) * (cfg.max_size - cfg.min_size));

        Asteroid ast;
        ast.Init(w, h, pos);
        ast.angular_vel = (Randf01(state) - 0.5f) * 0.4f;
        ast.velocity    = {(Randf01(state) - 0.5f) * 20.f,
                           (Randf01(state) - 0.5f) * 20.f};

        uint32_t ast_seed = Hash(seed ^ (uint32_t)positions.size());
        BuildAsteroid(ast, w, h, ast_seed, cfg.deep_mat_bias);
        field.push_back(std::move(ast));
    }
}
