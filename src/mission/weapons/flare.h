#pragma once
#include "mission/projectile.h"

class LightingSystem;

class FlareProjectile : public Projectile {
public:
    bool  landed            = false;
    int   light_id          = -1;
    int   attached_ast_id   = -1;
    Vector2 ast_local_offset = {0.f, 0.f};
    LightingSystem*   lighting = nullptr;

    void Draw() const override;
    void Update(float dt, std::vector<Asteroid>& asteroids,
                std::vector<Chunk>& chunks) override;
};
