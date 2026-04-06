#include "asteroid.h"
#include "raymath.h"
#include <queue>
#include <cmath>
#include <cstdlib>

static constexpr int MIN_CHUNK_TILES = 4;

void Asteroid::Init(int w, int h, Vector2 world_center) {
    cols   = w;
    rows   = h;
    center = world_center;
    rotation    = 0.f;
    angular_vel = 0.f;
    velocity    = {0.f, 0.f};
    cells.assign(rows, std::vector<Tile>(cols));
}

Vector2 Asteroid::TileWorldPos(int col, int row) const {
    float lx = (col - cols / 2.f + 0.5f) * TILE_SIZE;
    float ly = (row - rows / 2.f + 0.5f) * TILE_SIZE;
    float rx = lx * cosf(rotation) - ly * sinf(rotation);
    float ry = lx * sinf(rotation) + ly * cosf(rotation);
    return {center.x + rx, center.y + ry};
}

bool Asteroid::IsTileVisible(int col, int row) const {
    if (cells[row][col].material == TileMat::None) return false;
    if (cells[row][col].revealed) return true;
    // Surface-adjacent: any of the 4 neighbors is empty or out of bounds
    const int dc[] = {1,-1,0,0};
    const int dr[] = {0,0,1,-1};
    for (int i = 0; i < 4; i++) {
        int nc = col + dc[i], nr = row + dr[i];
        if (nc < 0 || nc >= cols || nr < 0 || nr >= rows) return true;
        if (cells[nr][nc].material == TileMat::None) return true;
    }
    return false;
}

bool Asteroid::IsAlive() const {
    for (auto& row : cells)
        for (auto& t : row)
            if (t.material != TileMat::None) return true;
    return false;
}

void Asteroid::Update(float dt) {
    rotation += angular_vel * dt;
    center.x += velocity.x * dt;
    center.y += velocity.y * dt;
}

void Asteroid::Draw() const {
    static const Color fog_color = {30, 30, 35, 255};

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            const Tile& t = cells[r][c];
            if (t.material == TileMat::None) continue;

            Vector2 wp = TileWorldPos(c, r);
            float half = TILE_SIZE * 0.5f;
            // Place rect so its top-left is at wp, pivot at center, rotate with asteroid
            Rectangle rect = {wp.x - half, wp.y - half, TILE_SIZE, TILE_SIZE};
            Vector2 origin = {half, half};

            Color col = IsTileVisible(c, r) ? MaterialColor(t.material) : fog_color;
            DrawRectanglePro(rect, origin, rotation * RAD2DEG, col);
        }
    }
}

void Asteroid::DamageTile(int col, int row, int dmg, int pen) {
    Tile& t = cells[row][col];
    if (t.material == TileMat::None) return;

    if (pen < t.armor) {
        // Ricochet or absorb — no HP damage handled by caller (projectile)
        return;
    }
    t.hp -= dmg;
    if (t.hp <= 0) {
        DestroyTile(col, row);
    }
}

void Asteroid::ExplosionDamage(Vector2 world_center, float radius, int dmg, int pen) {
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            if (cells[r][c].material == TileMat::None) continue;
            Vector2 tp = TileWorldPos(c, r);
            float dist = Vector2Distance(tp, world_center);
            if (dist <= radius) {
                float factor = 1.f - (dist / radius);
                int actual = (int)(dmg * factor * (1.f - cells[r][c].exp_resist));
                DamageTile(c, r, actual, pen);
            }
        }
    }
}

void Asteroid::DestroyTile(int col, int row) {
    Tile& t = cells[row][col];
    TileMat mat = t.material;
    Vector2 pos  = TileWorldPos(col, row);
    t.material   = TileMat::None;

    // Spawn chunk or dust
    if (on_spawn) {
        float roll = (float)rand() / RAND_MAX;
        bool is_chunk = roll < t.integrity;  // Note: t.material already cleared, use saved integrity
        // impulse: small random outward push
        Vector2 impulse = {
            ((float)rand()/RAND_MAX - 0.5f) * 80.f,
            ((float)rand()/RAND_MAX - 0.5f) * 80.f
        };
        on_spawn(pos, mat, impulse, is_chunk);
    }
}

int Asteroid::FloodFill(int sc, int sr, std::vector<std::vector<int>>& ids, int id) const {
    int count = 0;
    std::queue<std::pair<int,int>> q;
    q.push({sc, sr});
    ids[sr][sc] = id;
    const int dc[] = {1,-1,0,0};
    const int dr[] = {0,0,1,-1};
    while (!q.empty()) {
        auto [c, r] = q.front(); q.pop();
        count++;
        for (int i = 0; i < 4; i++) {
            int nc = c + dc[i], nr = r + dr[i];
            if (nc < 0 || nc >= cols || nr < 0 || nr >= rows) continue;
            if (ids[nr][nc] != -1) continue;
            if (cells[nr][nc].material == TileMat::None) continue;
            ids[nr][nc] = id;
            q.push({nc, nr});
        }
    }
    return count;
}

std::vector<std::vector<int>> Asteroid::GetIslands() const {
    std::vector<std::vector<int>> ids(rows, std::vector<int>(cols, -1));
    int id = 0;
    for (int r = 0; r < rows; r++)
        for (int c = 0; c < cols; c++)
            if (cells[r][c].material != TileMat::None && ids[r][c] == -1)
                FloodFill(c, r, ids, id++);
    return ids;
}

std::vector<Asteroid> Asteroid::CheckSplit() {
    std::vector<Asteroid> children;

    auto ids = GetIslands();

    // Count tiles per island
    std::vector<int> counts;
    for (int r = 0; r < rows; r++)
        for (int c = 0; c < cols; c++)
            if (ids[r][c] >= 0) {
                while ((int)counts.size() <= ids[r][c]) counts.push_back(0);
                counts[ids[r][c]]++;
            }

    if (counts.size() <= 1) return children;

    // Find the largest island — it stays in this asteroid
    int largest_id = 0;
    for (int i = 1; i < (int)counts.size(); i++)
        if (counts[i] > counts[largest_id]) largest_id = i;

    // Snapshot fields we need before any mutation
    float snap_angular_vel = angular_vel;
    Vector2 snap_velocity  = velocity;
    float snap_rotation    = rotation;
    int snap_cols          = cols;
    int snap_rows          = rows;
    Vector2 snap_center    = center;

    for (int island = 0; island < (int)counts.size(); island++) {
        if (island == largest_id) continue;

        if (counts[island] <= MIN_CHUNK_TILES) {
            for (int r = 0; r < snap_rows; r++)
                for (int c = 0; c < snap_cols; c++)
                    if (ids[r][c] == island) {
                        DestroyTile(c, r);
                        ids[r][c] = -1;
                    }
        } else {
            Asteroid child;
            child.on_spawn    = on_spawn;
            child.angular_vel = snap_angular_vel + ((float)rand()/RAND_MAX - 0.5f) * 0.5f;
            child.velocity    = {snap_velocity.x + ((float)rand()/RAND_MAX - 0.5f) * 30.f,
                                 snap_velocity.y + ((float)rand()/RAND_MAX - 0.5f) * 30.f};
            child.rotation    = snap_rotation;
            child.Init(snap_cols, snap_rows, snap_center);

            for (int r = 0; r < snap_rows; r++)
                for (int c = 0; c < snap_cols; c++)
                    if (ids[r][c] == island) {
                        child.cells[r][c] = cells[r][c];
                        cells[r][c].material = TileMat::None;
                    }

            children.push_back(std::move(child));
        }
    }

    return children;
}
