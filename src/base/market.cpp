#include "market.h"
#include <cstdlib>

static float Randf(float lo, float hi) {
    return lo + ((float)rand() / RAND_MAX) * (hi - lo);
}

void Market::Init() {
    auto add = [&](ResourceType r, int price) {
        entries[r] = {r, price, 1.f, 0.f};
    };
    add(ResourceType::Iron,          5);
    add(ResourceType::Copper,        15);
    add(ResourceType::Silicon,       12);
    add(ResourceType::Titanium,      40);
    add(ResourceType::Gold,          80);
    add(ResourceType::IronBar,       12);
    add(ResourceType::CopperWire,    35);
    add(ResourceType::SiliconWafer,  30);
    add(ResourceType::TitaniumPlate, 95);
    add(ResourceType::GoldIngot,     190);
}

void Market::ShuffleOnMissionReturn() {
    for (auto& [r, e] : entries) {
        float shift = Randf(-0.25f, 0.25f);
        e.demand = fmaxf(0.4f, fminf(2.5f, e.demand + shift));
        e.supply_pressure *= 0.7f;
    }
}

void Market::OnPlayerSell(ResourceType r, int qty) {
    auto it = entries.find(r);
    if (it != entries.end())
        it->second.supply_pressure += qty * 0.02f;
}

std::string Market::Save() const {
    // Returns a flat string of "type:demand:supply|..." for save system
    std::string out;
    for (auto& [r, e] : entries) {
        out += std::to_string((int)r) + ":" +
               std::to_string(e.demand) + ":" +
               std::to_string(e.supply_pressure) + "|";
    }
    return out;
}

void Market::Load(const std::string& data) {
    size_t pos = 0;
    while (pos < data.size()) {
        size_t end = data.find('|', pos);
        if (end == std::string::npos) break;
        std::string token = data.substr(pos, end - pos);
        pos = end + 1;
        auto c1 = token.find(':');
        auto c2 = token.find(':', c1 + 1);
        if (c1 == std::string::npos || c2 == std::string::npos) continue;
        int rt = std::stoi(token.substr(0, c1));
        float d  = std::stof(token.substr(c1 + 1, c2 - c1 - 1));
        float sp = std::stof(token.substr(c2 + 1));
        ResourceType r = (ResourceType)rt;
        if (entries.count(r)) {
            entries[r].demand          = d;
            entries[r].supply_pressure = sp;
        }
    }
}
