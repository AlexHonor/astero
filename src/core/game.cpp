#include "game.h"
#include "save.h"
#include "console.h"
#include "mission/mission_scene.h"
#include "base/base_scene.h"

// Destructor defined here where MissionScene and BaseScene are complete types
Game::Game()  = default;
Game::~Game() = default;

void Game::Init() {
    Console::Get();

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
    if (IsKeyPressed(KEY_GRAVE)) {
        Console::Get().Toggle();
    }

    if (Console::Get().IsOpen()) {
        int key = GetKeyPressed();
        if (key > 0) Console::Get().HandleInput(key);
        int ch = GetCharPressed();
        if (ch > 0) Console::Get().HandleTextInput(ch);
    } else {
        switch (current_scene) {
            case Scene::MISSION: mission->Update(dt); break;
            case Scene::BASE:    base->Update(dt);    break;
            default: break;
        }
    }
}

void Game::Draw() {
    switch (current_scene) {
        case Scene::MISSION: mission->Draw(); break;
        case Scene::BASE:    base->Draw();    break;
        default: break;
    }

    if (Console::Get().GetCVarValue<int>("fps", 0) == 1) {
        float fps = 1.0f / GetFrameTime();
        float ms = GetFrameTime() * 1000.f;
        Color col = fps >= 55.0f ? GREEN : (fps >= 30.0f ? YELLOW : RED);
        DrawText(TextFormat("FPS: %.0f  (%.1f ms)", fps, ms),
                 GetScreenWidth() - 160, 10, 14, col);
    }

    Console::Get().Draw();
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
