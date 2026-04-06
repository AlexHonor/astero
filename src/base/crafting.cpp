#include "crafting.h"

void Crafting::Init() {
    recipes.clear();
    auto add = [&](const char* name,
                   std::unordered_map<ResourceType,int> in,
                   std::unordered_map<ResourceType,int> out,
                   int cost) {
        recipes.push_back({name, in, out, cost});
    };

    add("Smelt Iron",
        {{ResourceType::Iron, 5}},
        {{ResourceType::IronBar, 3}}, 20);
    add("Smelt Copper",
        {{ResourceType::Copper, 5}},
        {{ResourceType::CopperWire, 3}}, 20);
    add("Silicon Wafer",
        {{ResourceType::Silicon, 4}},
        {{ResourceType::SiliconWafer, 2}}, 30);
    add("Titanium Plate",
        {{ResourceType::Titanium, 3}},
        {{ResourceType::TitaniumPlate, 2}}, 60);
    add("Gold Ingot",
        {{ResourceType::Gold, 3}},
        {{ResourceType::GoldIngot, 2}}, 50);
}

bool Crafting::CanCraft(int idx, const GameState& state) const {
    if (idx < 0 || idx >= (int)recipes.size()) return false;
    const Recipe& r = recipes[idx];
    if (state.money < r.craft_cost) return false;
    for (auto& [type, qty] : r.inputs) {
        auto it = state.inventory.find(type);
        if (it == state.inventory.end() || it->second < qty) return false;
    }
    return true;
}

bool Crafting::Craft(int idx, GameState& state) {
    if (!CanCraft(idx, state)) return false;
    const Recipe& r = recipes[idx];
    state.money -= r.craft_cost;
    for (auto& [type, qty] : r.inputs)
        state.inventory[type] -= qty;
    for (auto& [type, qty] : r.outputs)
        state.inventory[type] += qty;
    return true;
}
