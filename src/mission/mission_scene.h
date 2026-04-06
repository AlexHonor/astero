#pragma once
#include "raylib.h"
#include "core/game_state.h"
#include "mission/asteroid.h"
#include "mission/chunk.h"
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

    std::vector<Asteroid> asteroids;
    std::vector<Chunk>    chunks;
    Camera2D              cam = {0};

    void SpawnCallback(Vector2 pos, TileMat mat, Vector2 impulse, bool is_chunk);
};
