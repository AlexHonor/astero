#pragma once
#include "raylib.h"
#include "mission/bullet_tracer.h"
#include "mission/projectile.h"
#include "mission/weapons/explosive.h"
#include "mission/weapons/catcher.h"
#include "mission/weapons/flare.h"
#include "mission/weapons/intel_drone.h"
#include "core/game_state.h"
#include <array>
#include <memory>
#include <vector>

struct WeaponSlot {
    int   ammo         = -1;   // -1 = unlimited
    float cooldown     = 0.f;
    float cooldown_left = 0.f;
    bool  CanFire() const { return cooldown_left <= 0.f && ammo != 0; }
};

class WeaponManager {
public:
    std::vector<std::unique_ptr<Projectile>> projectiles;
    std::vector<Explosion>   explosions;
    std::array<WeaponSlot,9> slots;
    int  active_slot = 0;
    bool detonate_mines = false;

    ScanReport  scan_report;
    BulletTracer tracer;

    // Context set by MissionScene before each update
    std::vector<Asteroid>* asteroids = nullptr;
    std::vector<Chunk>*    chunks    = nullptr;
    Vector2*               ship_pos  = nullptr;

    void Init(const ShipConfig& cfg);
    void Update(float dt);
    void HandleInput(Vector2 ship_pos_val, Camera2D& cam);
    void Draw() const;
    void DrawHUD() const;

private:
    float ammo_mult    = 1.f;
    float cd_mult      = 1.f;
    int   pen_bonus    = 0;
    int   catcher_cap  = 5;

    void Fire(int slot, Vector2 origin, Vector2 direction);
    void SpawnFragment(Vector2 pos, Vector2 vel);
    void SpawnSubMunition(Vector2 pos, Vector2 vel, float er, int ed);

    // Mine detonation: set flag, picked up in Update
    std::vector<int> mine_indices;
};
