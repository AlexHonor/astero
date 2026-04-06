#pragma once
#include "raylib.h"
#include "core/game_state.h"

class Game;

class BaseScene {
public:
    void Init(Game* game, const MissionResult& result);
    void Update(float dt);
    void Draw();
    void Shutdown();

private:
    Game* game = nullptr;
    MissionResult last_result;
};
