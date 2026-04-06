#pragma once
#include <unordered_map>
#include <string>

enum class ResourceType {
    Rock, Iron, Copper, Silicon, Titanium, Gold,
    IronBar, CopperWire, SiliconWafer, TitaniumPlate, GoldIngot
};

struct ShipConfig {
    int   max_hp           = 100;
    int   max_cargo        = 50;
    float max_speed        = 300.f;
    float thrust           = 400.f;
    float ftl_charge_time  = 8.f;
    float flashlight_range = 350.f;
    int   drone_count      = 5;
    int   flare_count      = 8;
    float ammo_multiplier  = 1.f;
    float cooldown_mult    = 1.f;
    int   pen_bonus        = 0;
    int   catcher_capacity = 5;

    // Purchased upgrade flags
    bool hull1 = false, hull2 = false;
    bool cargo1 = false, cargo2 = false;
    bool engine_boost = false;
    bool ftl_cap = false;
    bool better_optics = false;
    bool extended_mag = false;
    bool fast_loader = false;
    bool pen_rounds = false;
    bool drone_pack = false;
    bool flare_pack = false;
    bool catcher_cap = false;

    static ShipConfig Default() { return ShipConfig{}; }
};

struct MissionResult {
    bool player_died = false;
    std::unordered_map<ResourceType, int> resources_collected;
    float ship_hp_fraction = 1.f;
};

class GameState {
public:
    int money = 500;
    int run_number = 1;
    std::unordered_map<ResourceType, int> inventory;
    ShipConfig ship_config;

    void AddResource(ResourceType r, int amount);
    bool SpendMoney(int cost);
    void ApplyMissionResult(const MissionResult& result);
    void ApplyDeathPenalty();

    void Reset() {
        money = 500;
        run_number = 1;
        inventory.clear();
        ship_config = ShipConfig::Default();
    }

    static GameState& Get() {
        static GameState instance;
        return instance;
    }

private:
    GameState() = default;
};
