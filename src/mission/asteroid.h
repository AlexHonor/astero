#pragma once
#include "raylib.h"
#include "tile.h"
#include "chunk.h"
#include <vector>
#include <functional>

class Asteroid {
public:
    // Grid stored row-major: cells[row][col]
    std::vector<std::vector<Tile>> cells;
    int cols = 0, rows = 0;

    int     id = -1;     // unique identity (assigned in Init)
    Vector2 center;      // world position of grid center
    float   rotation;    // radians
    float   angular_vel; // rad/s
    Vector2 velocity;    // px/s drift

    // Callback: called when a chunk or dust should be spawned
    std::function<void(Vector2 pos, TileMat mat, Vector2 impulse, bool is_chunk)> on_spawn;

    void Init(int w, int h, Vector2 world_center);
    void Update(float dt);
    void Draw() const;

    // Returns world position of center of tile (col, row)
    Vector2 TileWorldPos(int col, int row) const;

    bool IsTileVisible(int col, int row) const;
    bool IsAlive() const;

    void DamageTile(int col, int row, int dmg, int pen);
    void ExplosionDamage(Vector2 world_center, float radius, int dmg, int pen);

    // Island split check — returns newly spawned child asteroids
    std::vector<Asteroid> CheckSplit();

private:
    void DestroyTile(int col, int row);
    std::vector<std::vector<int>> GetIslands() const;  // returns island IDs per cell (-1 = none)
    int FloodFill(int start_col, int start_row, std::vector<std::vector<int>>& ids, int id) const;
};
