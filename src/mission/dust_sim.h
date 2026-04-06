#pragma once
#include "raylib.h"
#include "mission/tile.h"
#include <unordered_set>
#include <vector>

struct DustCell {
    float   density = 0.f;
    Color   color   = {80, 80, 85, 255};
    float   vx = 0.f, vy = 0.f;   // drift in cells/step
};

class DustSim {
public:
    static constexpr int   CELL_SIZE  = 8;
    static constexpr int   GRID_W     = 500;   // 500*8 = 4000px
    static constexpr int   GRID_H     = 500;
    static constexpr float SIM_HZ     = 10.f;
    static constexpr int   MAX_ACTIVE = 8000;

    void Init(int screen_w, int screen_h);
    void Shutdown();

    void AddDust(Vector2 world_pos, TileMat mat, Vector2 impulse);
    void AddExplosionDust(Vector2 world_center, float radius, TileMat mat);

    void Update(float dt);
    void Draw(Camera2D& cam);

    float DensityAt(Vector2 world_pos) const;

private:
    std::vector<DustCell>    grid;       // flat [row * GRID_W + col]
    std::unordered_set<int>  active;

    RenderTexture2D tex = {0};
    float sim_timer = 0.f;
    int sw = 0, sh = 0;

    bool InBounds(int col, int row) const {
        return col >= 0 && col < GRID_W && row >= 0 && row < GRID_H;
    }

    static Color MatDustColor(TileMat m);
    void Step();
};
