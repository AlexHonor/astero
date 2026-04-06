#include "mission_scene.h"
#include "core/game.h"

void MissionScene::Init(Game* g, const ShipConfig& cfg) {
    game = g;
    (void)cfg;
}

void MissionScene::Update(float dt) {
    (void)dt;
    // Stub: press Escape to return to base
    if (IsKeyPressed(KEY_ESCAPE)) {
        MissionResult result;
        result.player_died = false;
        game->EndMission(result);
    }
}

void MissionScene::Draw() {
    DrawText("MISSION MODE (stub)", 50, 50, 24, WHITE);
    DrawText("Press ESC to return to base", 50, 90, 16, GRAY);
}

void MissionScene::Shutdown() {}
