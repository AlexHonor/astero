#pragma once
#include "raylib.h"
#include "core/game_state.h"

class Game;

class MissionScene {
public:
    void Init(Game* game, const ShipConfig& cfg);
    void Update(float dt);
    void Draw();
    void Shutdown();

private:
    Game* game = nullptr;
};
