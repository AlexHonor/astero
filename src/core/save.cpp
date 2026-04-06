#include "save.h"
#include "game_state.h"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <string>

static const std::unordered_map<ResourceType, std::string> kResNames = {
    {ResourceType::Rock,          "rock"},
    {ResourceType::Iron,          "iron"},
    {ResourceType::Copper,        "copper"},
    {ResourceType::Silicon,       "silicon"},
    {ResourceType::Titanium,      "titanium"},
    {ResourceType::Gold,          "gold"},
    {ResourceType::IronBar,       "iron_bar"},
    {ResourceType::CopperWire,    "copper_wire"},
    {ResourceType::SiliconWafer,  "silicon_wafer"},
    {ResourceType::TitaniumPlate, "titanium_plate"},
    {ResourceType::GoldIngot,     "gold_ingot"},
};

static const std::unordered_map<std::string, ResourceType> kResTypes = []() {
    std::unordered_map<std::string, ResourceType> m;
    for (auto& [k, v] : kResNames) m[v] = k;
    return m;
}();

#define WRITE_BOOL(f, name, val) (f) << (name) << "=" << ((val) ? 1 : 0) << "\n"
#define WRITE_INT(f, name, val)  (f) << (name) << "=" << (val) << "\n"
#define WRITE_FLT(f, name, val)  (f) << (name) << "=" << (val) << "\n"

void SaveSystem::Save(const GameState& s, const char* path) {
    std::ofstream f(path);
    if (!f) return;

    WRITE_INT(f, "money", s.money);
    WRITE_INT(f, "run_number", s.run_number);

    for (auto& [type, count] : s.inventory) {
        if (count > 0)
            f << "res." << kResNames.at(type) << "=" << count << "\n";
    }

    const ShipConfig& c = s.ship_config;
    WRITE_BOOL(f, "up.hull1",        c.hull1);
    WRITE_BOOL(f, "up.hull2",        c.hull2);
    WRITE_BOOL(f, "up.cargo1",       c.cargo1);
    WRITE_BOOL(f, "up.cargo2",       c.cargo2);
    WRITE_BOOL(f, "up.engine_boost", c.engine_boost);
    WRITE_BOOL(f, "up.ftl_cap",      c.ftl_cap);
    WRITE_BOOL(f, "up.better_optics",c.better_optics);
    WRITE_BOOL(f, "up.extended_mag", c.extended_mag);
    WRITE_BOOL(f, "up.fast_loader",  c.fast_loader);
    WRITE_BOOL(f, "up.pen_rounds",   c.pen_rounds);
    WRITE_BOOL(f, "up.drone_pack",   c.drone_pack);
    WRITE_BOOL(f, "up.flare_pack",   c.flare_pack);
    WRITE_BOOL(f, "up.catcher_cap",  c.catcher_cap);
}

bool SaveSystem::Load(GameState& s, const char* path) {
    std::ifstream f(path);
    if (!f) return false;

    std::string line;
    while (std::getline(f, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);

        auto ival = [&]() { return std::stoi(val); };
        auto fval = [&]() { return std::stof(val); };
        auto bval = [&]() { return val == "1"; };

        if (key == "money")      { s.money = ival(); }
        else if (key == "run_number") { s.run_number = ival(); }
        else if (key.substr(0, 4) == "res.") {
            std::string res_name = key.substr(4);
            auto it = kResTypes.find(res_name);
            if (it != kResTypes.end()) s.inventory[it->second] = ival();
        }
        else if (key == "up.hull1")         { s.ship_config.hull1 = bval(); }
        else if (key == "up.hull2")         { s.ship_config.hull2 = bval(); }
        else if (key == "up.cargo1")        { s.ship_config.cargo1 = bval(); }
        else if (key == "up.cargo2")        { s.ship_config.cargo2 = bval(); }
        else if (key == "up.engine_boost")  { s.ship_config.engine_boost = bval(); }
        else if (key == "up.ftl_cap")       { s.ship_config.ftl_cap = bval(); }
        else if (key == "up.better_optics") { s.ship_config.better_optics = bval(); }
        else if (key == "up.extended_mag")  { s.ship_config.extended_mag = bval(); }
        else if (key == "up.fast_loader")   { s.ship_config.fast_loader = bval(); }
        else if (key == "up.pen_rounds")    { s.ship_config.pen_rounds = bval(); }
        else if (key == "up.drone_pack")    { s.ship_config.drone_pack = bval(); }
        else if (key == "up.flare_pack")    { s.ship_config.flare_pack = bval(); }
        else if (key == "up.catcher_cap")   { s.ship_config.catcher_cap = bval(); }
    }

    // Re-apply upgrade effects to derived stats
    ShipConfig& c = s.ship_config;
    if (c.hull1)         c.max_hp           += 50;
    if (c.hull2)         c.max_hp           += 100;
    if (c.cargo1)        c.max_cargo        += 50;
    if (c.cargo2)        c.max_cargo        += 100;
    if (c.engine_boost)  { c.max_speed += 100; c.thrust += 100; }
    if (c.ftl_cap)       c.ftl_charge_time  -= 2.f;
    if (c.better_optics) c.flashlight_range += 100.f;
    if (c.extended_mag)  c.ammo_multiplier   = 1.5f;
    if (c.fast_loader)   c.cooldown_mult     = 0.8f;
    if (c.pen_rounds)    c.pen_bonus         = 1;
    if (c.drone_pack)    c.drone_count       += 5;
    if (c.flare_pack)    c.flare_count       += 8;
    if (c.catcher_cap)   c.catcher_capacity  = 10;

    return true;
}
