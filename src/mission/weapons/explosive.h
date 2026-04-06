#pragma once
#include "mission/projectile.h"

// Used by ExplosiveShell, ShrapnelBullet sub-fragments, ClusterBomb sub-munitions
struct Explosion {
    Vector2 pos;
    float   radius;
    int     damage;
    int     pen;
    float   timer = 0.4f;   // visual lifetime

    void Apply(std::vector<Asteroid>& asteroids);
    void Draw() const;
};

// Explosive shell: explodes on tile hit
class ExplosiveShell : public Projectile {
public:
    float exp_radius = 80.f;
    int   exp_damage = 40;
    std::vector<Explosion>* explosions = nullptr;

    void Draw() const override;
    void Update(float dt, std::vector<Asteroid>& asteroids,
                std::vector<Chunk>& chunks) override;
};

// Shrapnel bullet: splits after fuse_time into fragments
class ShrapnelBullet : public Projectile {
public:
    float fuse_time     = 0.5f;
    float fuse_elapsed  = 0.f;
    int   frag_count    = 6;
    // Fragments are spawned into the weapon manager's projectile list
    std::function<void(Vector2 pos, Vector2 vel)> spawn_fragment;

    void Draw() const override;
    void Update(float dt, std::vector<Asteroid>& asteroids,
                std::vector<Chunk>& chunks) override;
};

// Cluster bomb: arm_time, then splits into sub-munitions
class ClusterBomb : public Projectile {
public:
    float arm_time     = 0.8f;
    float arm_elapsed  = 0.f;
    int   sub_count    = 5;
    float sub_exp_r    = 40.f;
    int   sub_exp_dmg  = 25;
    std::vector<Explosion>* explosions = nullptr;
    std::function<void(Vector2 pos, Vector2 vel, float er, int ed)> spawn_sub;

    void Draw() const override;
    void Update(float dt, std::vector<Asteroid>& asteroids,
                std::vector<Chunk>& chunks) override;
};

// Timed mine: sticks to surface, detonates after fuse or on signal
class TimedMine : public Projectile {
public:
    float  fuse_time         = 2.f;
    float  fuse_elapsed      = 0.f;
    bool   embedded          = false;
    float  exp_radius        = 120.f;
    int    exp_damage        = 60;
    bool   detonate_now      = false;
    int    attached_ast_id   = -1;
    Vector2 ast_local_offset = {0.f, 0.f};
    std::vector<Explosion>* explosions = nullptr;

    void Draw() const override;
    void Update(float dt, std::vector<Asteroid>& asteroids,
                std::vector<Chunk>& chunks) override;
};
