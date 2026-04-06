#pragma once
#include "mission/projectile.h"
#include "mission/tile.h"
#include <unordered_map>
#include <string>

struct ScanReport {
    std::unordered_map<TileMat, int> tile_counts;
    float display_timer = 5.f;
    bool  has_data      = false;
};

class IntelDrone : public Projectile {
public:
    bool         landed           = false;
    float        scan_radius      = 150.f;
    ScanReport*  report           = nullptr;   // owned by WeaponManager / MissionScene
    int          attached_ast_id  = -1;
    Vector2      ast_local_offset = {0.f, 0.f};

    void Draw() const override;
    void Update(float dt, std::vector<Asteroid>& asteroids,
                std::vector<Chunk>& chunks) override;

private:
    void DoScan(std::vector<Asteroid>& asteroids);
};
