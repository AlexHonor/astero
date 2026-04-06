#include "quests.h"
#include <cstdlib>
#include <algorithm>

static const ResourceType kQuestable[] = {
    ResourceType::Iron, ResourceType::Copper, ResourceType::Silicon,
    ResourceType::Titanium, ResourceType::Gold,
    ResourceType::IronBar, ResourceType::CopperWire
};
static const char* kQuestableNames[] = {
    "Iron", "Copper", "Silicon", "Titanium", "Gold", "Iron Bar", "Copper Wire"
};
static const int kBaseValues[] = {5, 15, 12, 40, 80, 12, 35};
static constexpr int N_TYPES = 7;

void QuestManager::GenerateQuests(int run_number, int count) {
    quests.clear();
    for (int i = 0; i < count; i++) {
        int t_idx = rand() % N_TYPES;
        ResourceType r = kQuestable[t_idx];
        int base_amt   = 8 + run_number * 3 + rand() % 10;
        int reward     = (int)(base_amt * kBaseValues[t_idx] * (1.2f + run_number * 0.1f));

        Quest q;
        q.description  = std::string("Deliver ") + std::to_string(base_amt) +
                         " units of " + kQuestableNames[t_idx];
        q.resource     = r;
        q.required     = base_amt;
        q.reward_money = reward;
        q.missions_left = 3;
        quests.push_back(q);
    }
}

bool QuestManager::TryComplete(int idx, GameState& state) {
    if (idx < 0 || idx >= (int)quests.size()) return false;
    Quest& q = quests[idx];
    if (q.completed) return false;
    if (state.inventory[q.resource] < q.required) return false;

    state.inventory[q.resource] -= q.required;
    state.money += q.reward_money;
    q.completed = true;
    return true;
}

void QuestManager::OnMissionReturn() {
    for (auto& q : quests)
        if (!q.completed) q.missions_left--;
    quests.erase(
        std::remove_if(quests.begin(), quests.end(),
                       [](const Quest& q){ return q.completed || q.missions_left <= 0; }),
        quests.end());
}
