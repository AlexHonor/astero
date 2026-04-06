#pragma once
#include "raylib.h"
#include "raymath.h"
#include "core/game_state.h"
#include <cmath>

class Ship {
public:
    Vector2 pos;
    Vector2 vel;
    float   rotation = 0.f;   // radians, faces mouse

    int hp, max_hp;
    int cargo, max_cargo;

    float thrust;
    float max_speed;
    float damping = 2.5f;

    bool  alive             = true;
    float damage_flash      = 0.f;   // countdown for red tint

    void Init(const ShipConfig& cfg, Vector2 start_pos);
    void Update(float dt, Camera2D& cam);
    void Draw() const;
    void TakeDamage(int dmg);
    void UpdateFlash(float dt);

    // Returns a rectangle in world space representing the ship's collision box
    Rectangle Bounds() const {
        return {pos.x - 10.f, pos.y - 10.f, 20.f, 20.f};
    }
};
