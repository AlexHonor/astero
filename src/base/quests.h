#pragma once
#include "core/game_state.h"
#include <vector>
#include <string>

struct Quest {
    std::string  description;
    ResourceType resource;
    int          required;
    int          reward_money;
    int          missions_left = 3;
    bool         completed     = false;
};

class QuestManager {
public:
    std::vector<Quest> quests;

    void GenerateQuests(int run_number, int count = 4);
    bool TryComplete(int idx, GameState& state);
    void OnMissionReturn();   // decrement missions_left, remove expired
};
