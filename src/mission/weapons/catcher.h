#pragma once
#include "mission/projectile.h"

// Catcher: passes through tiles, tethers nearby chunks
class CatcherProjectile : public Projectile {
public:
    float  detect_radius  = 400.f;
    int    max_tethers    = 5;
    float  tether_speed   = 150.f;
    Vector2 ship_pos      = {0, 0};

    // Tethered chunk indices into the chunks vector
    std::vector<int> tethered;

    void Draw() const override;
    void Update(float dt, std::vector<Asteroid>& asteroids,
                std::vector<Chunk>& chunks) override;
};
