#include "base_scene.h"
#include "core/game.h"

void BaseScene::Init(Game* g, const MissionResult& result) {
    game = g;
    last_result = result;
    GameState::Get().ApplyMissionResult(result);
}

void BaseScene::Update(float dt) {
    (void)dt;
    // Stub: press Enter to launch a mission
    if (IsKeyPressed(KEY_ENTER)) {
        game->StartMission();
    }
}

void BaseScene::Draw() {
    GameState& s = GameState::Get();
    DrawText("BASE MODE (stub)", 50, 50, 24, WHITE);
    DrawText(TextFormat("Money: %d cr", s.money), 50, 90, 18, GOLD);
    DrawText("Press ENTER to launch mission", 50, 120, 16, GRAY);
    if (last_result.player_died)
        DrawText("SHIP LOST - all upgrades reset", 50, 160, 18, RED);
}

void BaseScene::Shutdown() {}
