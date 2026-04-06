#include "game.h"
#include "save.h"
#include "mission/mission_scene.h"
#include "base/base_scene.h"

// Destructor defined here where MissionScene and BaseScene are complete types
Game::Game()  = default;
Game::~Game() = default;

void Game::Init() {
    // Try to load existing save; start fresh if none
    if (!SaveSystem::Load(GameState::Get())) {
        GameState::Get().Reset();
    }

    // Start in base
    base = std::make_unique<BaseScene>();
    MissionResult empty{};
    base->Init(this, empty);
    current_scene = Scene::BASE;
}

void Game::Update(float dt) {
    if (IsKeyPressed(KEY_GRAVE)) show_fps = !show_fps;

    switch (current_scene) {
        case Scene::MISSION: mission->Update(dt); break;
        case Scene::BASE:    base->Update(dt);    break;
        default: break;
    }
}

void Game::Draw() {
    switch (current_scene) {
        case Scene::MISSION: mission->Draw(); break;
        case Scene::BASE:    base->Draw();    break;
        default: break;
    }

    if (show_fps) {
        int fps = GetFPS();
        float ms = 1000.f / (fps > 0 ? fps : 1);
        Color col = fps >= 55 ? GREEN : (fps >= 30 ? YELLOW : RED);
        DrawText(TextFormat("FPS: %d  (%.1f ms)", fps, ms),
                 GetScreenWidth() - 180, GetScreenHeight() - 22, 14, col);
    }
}

void Game::Shutdown() {
    if (mission) mission->Shutdown();
    if (base)    base->Shutdown();
    SaveSystem::Save(GameState::Get());
}

void Game::StartMission() {
    if (mission) mission->Shutdown();
    mission = std::make_unique<MissionScene>();
    mission->Init(this, GameState::Get().ship_config);
    current_scene = Scene::MISSION;
}

void Game::EndMission(const MissionResult& result) {
    if (mission) { mission->Shutdown(); mission.reset(); }

    base = std::make_unique<BaseScene>();
    base->Init(this, result);
    current_scene = Scene::BASE;

    SaveSystem::Save(GameState::Get());
}
