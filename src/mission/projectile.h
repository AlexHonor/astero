#pragma once
#include "raylib.h"
#include "raymath.h"
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

protected:
    // Returns true and calls OnHitTile if a tile was hit along vel direction
    bool CheckTileHit(std::vector<Asteroid>& asteroids);
};
