#pragma once
#include "core/game_state.h"
#include <vector>
#include <unordered_map>
#include <string>

struct Recipe {
    std::string name;
    std::unordered_map<ResourceType, int> inputs;
    std::unordered_map<ResourceType, int> outputs;
    int craft_cost = 0;
};

class Crafting {
public:
    std::vector<Recipe> recipes;
    void Init();
    bool CanCraft(int idx, const GameState& state) const;
    bool Craft(int idx, GameState& state);
};
