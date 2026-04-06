#pragma once
#include "raylib.h"
#include "mission/asteroid.h"
#include "mission/chunk.h"
#include "mission/ship.h"
#include <vector>

class Minimap {
public:
    static constexpr int   MAP_SIZE   = 180;
    static constexpr float WORLD_SIZE = 4000.f;

    void Init();
    void Shutdown();
    void Update(const std::vector<Asteroid>& asteroids,
                const std::vector<Chunk>& chunks,
                const Ship& ship);
    void Draw(int screen_x, int screen_y) const;

private:
    RenderTexture2D tex = {0};
    float update_timer  = 0.f;
    static constexpr float UPDATE_HZ = 10.f;
};
