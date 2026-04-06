#pragma once
#include "raylib.h"
#include "tile.h"
#include <cmath>

struct Chunk {
    Vector2  pos;
    Vector2  vel;
    TileMat material;
    bool     alive = true;

    void Update(float dt) {
        pos.x += vel.x * dt;
        pos.y += vel.y * dt;
        vel.x *= (1.f - 0.5f * dt);
        vel.y *= (1.f - 0.5f * dt);
    }

    void Draw() const {
        Color c = MaterialColor(material);
        c.r = (unsigned char)fminf(255, c.r * 1.4f);
        c.g = (unsigned char)fminf(255, c.g * 1.4f);
        c.b = (unsigned char)fminf(255, c.b * 1.4f);
        DrawRectangle((int)(pos.x - 4), (int)(pos.y - 4), 8, 8, c);
    }
};
