#pragma once
#include "game_state.h"
#include <memory>

// Forward declarations to avoid heavy includes in this header
class MissionScene;
class BaseScene;

enum class Scene { MAIN_MENU, MISSION, BASE };

class Game {
public:
    Game();
    ~Game();

    void Init();
    void Update(float dt);
    void Draw();
    void Shutdown();

    // Called by scenes to trigger transitions
    void StartMission();
    void EndMission(const MissionResult& result);

private:
    Scene current_scene = Scene::BASE;
    std::unique_ptr<MissionScene> mission;
    std::unique_ptr<BaseScene>    base;
};
