#pragma once
#include "core/game_state.h"
#include <unordered_map>

struct MarketEntry {
    ResourceType type;
    int   base_price;
    float demand           = 1.f;
    float supply_pressure  = 0.f;

    int BuyPrice()  const { return (int)(base_price * demand / (1.f + supply_pressure * 0.3f)); }
    int SellPrice() const { return (int)(BuyPrice() * 1.25f); }
};

class Market {
public:
    std::unordered_map<ResourceType, MarketEntry> entries;

    void Init();
    void ShuffleOnMissionReturn();
    void OnPlayerSell(ResourceType r, int qty);

    void Load(const std::string& data);
    std::string Save() const;
};
