#pragma once
#include "raylib.h"
#include "core/game_state.h"
#include "mission/asteroid.h"
#include "mission/chunk.h"
#include "mission/ship.h"
#include "mission/weapon_manager.h"
#include "mission/minimap.h"
#include "mission/lighting.h"
#include "mission/dust_sim.h"
#include <vector>

class Game;

class MissionScene {
public:
    void Init(Game* game, const ShipConfig& cfg);
    void Update(float dt);
    void Draw();
    void Shutdown();

private:
    Game* game = nullptr;

    Ship                  ship;
    WeaponManager         weapons;
    Minimap               minimap;
    LightingSystem        lighting;
    DustSim               dust;
    int                   ship_light_id  = -1;
    int                   ambient_light_id = -1;
    std::vector<Asteroid> asteroids;
    std::vector<Chunk>    chunks;
    Camera2D              cam = {0};

    float mission_timer = 0.f;
    float boundary_dmg_timer = 0.f;

    void SpawnCallback(Vector2 pos, TileMat mat, Vector2 impulse, bool is_chunk);
    void CheckShipAsteroidCollision();
    void CollectNearbyChunks();
    void DrawHUD() const;
    void RewireSpawnCallbacks();
};
