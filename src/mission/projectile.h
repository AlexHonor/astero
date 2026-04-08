#pragma once
#include "raylib.h"
#include "raymath.h"
#include "mission/bullet_tracer.h"
#include <vector>
#include <functional>

class Asteroid;
class Chunk;

class Projectile {
public:
    Vector2 pos;
    Vector2 vel;
    int     penetration = 1;
    int     damage      = 10;
    float   lifetime    = 3.f;
    bool    alive       = true;

    virtual ~Projectile() = default;
    virtual void Update(float dt, std::vector<Asteroid>& asteroids,
                        std::vector<Chunk>& chunks);
    virtual void Draw() const;

    void Kill() { alive = false; }

    // Set by WeaponManager before first update to enable recording
    BulletTracer* tracer     = nullptr;
    BulletTrace   trace_data;

protected:
    Vector2 prev_pos = {0.f, 0.f};
    bool    tracing  = false;

    // Raycast from prev_pos to pos; hits first tile found. Returns true on hit.
    bool CheckTileHit(std::vector<Asteroid>& asteroids);
};
