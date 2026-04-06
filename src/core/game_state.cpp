#include "game_state.h"

void GameState::AddResource(ResourceType r, int amount) {
    inventory[r] += amount;
}

bool GameState::SpendMoney(int cost) {
    if (money < cost) return false;
    money -= cost;
    return true;
}

void GameState::ApplyMissionResult(const MissionResult& result) {
    if (result.player_died) {
        ApplyDeathPenalty();
        return;
    }
    for (auto& [type, amount] : result.resources_collected) {
        AddResource(type, amount);
    }
}

void GameState::ApplyDeathPenalty() {
    // Resources from the mission are already lost (never merged)
    // Reset ship to default, keep money
    ship_config = ShipConfig::Default();
}
