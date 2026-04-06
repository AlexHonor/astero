#include "base_scene.h"
#include "core/game.h"
#include "core/save.h"
#include <cstring>
#include <algorithm>

static const char* kResNames[] = {
    "Rock","Iron","Copper","Silicon","Titanium","Gold",
    "Iron Bar","Copper Wire","Silicon Wafer","Ti Plate","Gold Ingot"
};

bool BaseScene::Button(int x, int y, int w, int h,
                       const char* label, Color bg, Color fg) {
    Rectangle r = {(float)x, (float)y, (float)w, (float)h};
    bool hover = CheckCollisionPointRec(GetMousePosition(), r);
    Color draw_bg = hover ? Color{(unsigned char)fminf(255,bg.r+30),
                                  (unsigned char)fminf(255,bg.g+30),
                                  (unsigned char)fminf(255,bg.b+30), bg.a} : bg;
    DrawRectangleRec(r, draw_bg);
    DrawRectangleLinesEx(r, 1, hover ? WHITE : Color{100,100,100,255});
    int tw = MeasureText(label, 14);
    DrawText(label, x + (w - tw) / 2, y + (h - 14) / 2, 14, fg);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void BaseScene::Init(Game* g, const MissionResult& result) {
    game        = g;
    last_result = result;
    GameState::Get().ApplyMissionResult(result);

    if (result.player_died) {
        // Ensure quests aren't lost but on-mission-return called
    } else if (!result.resources_collected.empty() || result.ship_hp_fraction < 1.f) {
        market.ShuffleOnMissionReturn();
        quests.OnMissionReturn();
        GameState::Get().run_number++;
    }

    // First time init of market/quests/crafting
    if (market.entries.empty())  market.Init();
    if (quests.quests.empty())   quests.GenerateQuests(GameState::Get().run_number);
    crafting.Init();

    status_msg   = "";
    status_timer = 0.f;
    tab          = BaseTab::Inventory;
    memset(market_qty, 0, sizeof(market_qty));

    if (result.player_died)
        SetStatus("SHIP LOST - all upgrades reset, money retained");
}

void BaseScene::Update(float dt) {
    if (status_timer > 0) status_timer -= dt;
}

void BaseScene::Draw() {
    ClearBackground(Color{8, 8, 18, 255});

    DrawText("ASTEROID RUSH - BASE", 10, 8, 20, WHITE);
    DrawText(TextFormat("Run #%d    Money: %d cr",
             GameState::Get().run_number,
             GameState::Get().money), 10, 34, 16, GOLD);

    DrawTabBar();

    int content_y = 90;
    switch (tab) {
        case BaseTab::Inventory:    DrawInventory();     break;
        case BaseTab::Market:       DrawMarket();        break;
        case BaseTab::Quests:       DrawQuests();        break;
        case BaseTab::Crafting:     DrawCrafting();      break;
        case BaseTab::Ship:         DrawShipUpgrades();  break;
    }
    (void)content_y;

    DrawLaunchButton();

    // Status message
    if (status_timer > 0) {
        Color sc = (status_msg.find("LOST") != std::string::npos ||
                    status_msg.find("Cannot") != std::string::npos) ? RED : GREEN;
        DrawText(status_msg.c_str(), 10, GetScreenHeight() - 50, 15, sc);
    }
}

void BaseScene::DrawTabBar() {
    const char* tab_names[] = {"INVENTORY","MARKET","QUESTS","CRAFTING","SHIP"};
    BaseTab tab_vals[] = {BaseTab::Inventory, BaseTab::Market,
                          BaseTab::Quests,    BaseTab::Crafting, BaseTab::Ship};
    int tx = 10, ty = 60;
    for (int i = 0; i < 5; i++) {
        bool active = (tab == tab_vals[i]);
        Color bg = active ? Color{60,80,120,255} : Color{30,30,50,200};
        if (Button(tx, ty, 130, 26, tab_names[i], bg, WHITE))
            tab = tab_vals[i];
        tx += 135;
    }
}

void BaseScene::DrawInventory() {
    DrawText("--- INVENTORY ---", 10, 94, 16, LIGHTGRAY);
    int x = 10, y = 120;
    GameState& s = GameState::Get();
    for (int i = 1; i < 11; i++) {   // skip Rock (index 0)
        ResourceType r = (ResourceType)i;
        int qty = 0;
        auto it = s.inventory.find(r);
        if (it != s.inventory.end()) qty = it->second;
        if (qty <= 0) continue;
        DrawText(TextFormat("%-14s x%d", kResNames[i], qty), x, y, 15, WHITE);
        y += 22;
        if (y > GetScreenHeight() - 80) { x += 280; y = 120; }
    }
    if (y == 120 && x == 10)
        DrawText("(empty)", 10, 120, 15, DARKGRAY);
}

void BaseScene::DrawMarket() {
    DrawText("--- MARKET ---", 10, 94, 16, LIGHTGRAY);
    DrawText("Resource        Buy     Sell    Qty  Action", 10, 120, 13, GRAY);
    DrawLine(10, 135, 700, 135, DARKGRAY);

    GameState& s = GameState::Get();
    int y = 142;
    int idx = 0;
    for (auto& [r, entry] : market.entries) {
        int inv_qty = 0;
        auto it = s.inventory.find(r);
        if (it != s.inventory.end()) inv_qty = it->second;

        int ri = (int)r;
        const char* name = (ri >= 0 && ri < 11) ? kResNames[ri] : "?";
        DrawText(TextFormat("%-14s", name), 10, y, 13, WHITE);
        DrawText(TextFormat("%4d cr", entry.BuyPrice()),  180, y, 13, GREEN);
        DrawText(TextFormat("%4d cr", entry.SellPrice()), 260, y, 13, RED);
        DrawText(TextFormat("x%-4d", inv_qty), 350, y, 13, LIGHTGRAY);

        // Qty controls
        if (Button(400, y-2, 20, 18, "-", {50,30,30,200}, WHITE))
            if (market_qty[idx] > 0) market_qty[idx]--;
        DrawText(TextFormat("%d", market_qty[idx]), 424, y, 13, WHITE);
        if (Button(450, y-2, 20, 18, "+", {30,50,30,200}, WHITE))
            if (market_qty[idx] < inv_qty) market_qty[idx]++;

        if (Button(480, y-2, 60, 18, "SELL", {30,80,30,200}, WHITE)) {
            int qty = market_qty[idx];
            if (qty > 0 && inv_qty >= qty) {
                int earn = entry.BuyPrice() * qty;
                s.inventory[r] -= qty;
                s.money += earn;
                market.OnPlayerSell(r, qty);
                market_qty[idx] = 0;
                SetStatus(TextFormat("Sold %d %s for %d cr", qty, name, earn));
                SaveSystem::Save(s);
            } else {
                SetStatus("Cannot sell: insufficient quantity");
            }
        }
        if (Button(548, y-2, 60, 18, "BUY", {30,30,80,200}, WHITE)) {
            int qty = market_qty[idx];
            int cost = entry.SellPrice() * qty;
            if (qty > 0 && s.money >= cost) {
                s.money -= cost;
                s.inventory[r] += qty;
                market_qty[idx] = 0;
                SetStatus(TextFormat("Bought %d %s for %d cr", qty, name, cost));
                SaveSystem::Save(s);
            } else {
                SetStatus("Cannot buy: insufficient funds");
            }
        }

        y += 24;
        idx++;
    }
}

void BaseScene::DrawQuests() {
    DrawText("--- QUESTS ---", 10, 94, 16, LIGHTGRAY);
    if (quests.quests.empty()) {
        DrawText("No active quests. Return from a mission to get new ones.", 10, 120, 14, DARKGRAY);
        return;
    }
    int y = 120;
    for (int i = 0; i < (int)quests.quests.size(); i++) {
        Quest& q = quests.quests[i];
        Color c = q.completed ? DARKGRAY : WHITE;
        DrawText(q.description.c_str(), 10, y, 14, c);
        DrawText(TextFormat("Reward: %d cr  | Expires in %d run(s)",
                 q.reward_money, q.missions_left), 10, y+18, 12, GRAY);

        GameState& s = GameState::Get();
        int have = 0;
        auto it = s.inventory.find(q.resource);
        if (it != s.inventory.end()) have = it->second;
        DrawText(TextFormat("Have: %d/%d", have, q.required), 400, y, 12,
                 have >= q.required ? GREEN : ORANGE);

        if (!q.completed) {
            if (Button(500, y, 100, 22, "COMPLETE", {40,80,40,220}, WHITE)) {
                if (quests.TryComplete(i, GameState::Get())) {
                    SetStatus(TextFormat("Quest complete! +%d cr", q.reward_money));
                    SaveSystem::Save(GameState::Get());
                } else {
                    SetStatus("Cannot complete: insufficient resources");
                }
            }
        }
        y += 56;
    }
}

void BaseScene::DrawCrafting() {
    DrawText("--- CRAFTING ---", 10, 94, 16, LIGHTGRAY);
    int y = 120;
    GameState& s = GameState::Get();
    for (int i = 0; i < (int)crafting.recipes.size(); i++) {
        const Recipe& r = crafting.recipes[i];
        bool can = crafting.CanCraft(i, s);
        Color nc = can ? WHITE : DARKGRAY;

        DrawText(r.name.c_str(), 10, y, 15, nc);
        // Inputs
        std::string inputs = "  Needs: ";
        for (auto& [t, q2] : r.inputs)
            inputs += std::to_string(q2) + "x " + kResNames[(int)t] + "  ";
        DrawText(inputs.c_str(), 10, y+17, 12, GRAY);
        // Outputs
        std::string outputs = "  Makes: ";
        for (auto& [t, q2] : r.outputs)
            outputs += std::to_string(q2) + "x " + kResNames[(int)t] + "  ";
        DrawText(outputs.c_str(), 10, y+31, 12, LIGHTGRAY);
        if (r.craft_cost > 0)
            DrawText(TextFormat("  Cost: %d cr", r.craft_cost), 300, y+17, 12, GOLD);

        if (Button(530, y+10, 80, 24, "CRAFT", can ? Color{40,80,40,220} : DARKGRAY, WHITE)) {
            if (crafting.Craft(i, s)) {
                SetStatus(TextFormat("Crafted: %s", r.name.c_str()));
                SaveSystem::Save(s);
            } else {
                SetStatus("Cannot craft: insufficient resources or funds");
            }
        }
        y += 60;
    }
}

void BaseScene::DrawShipUpgrades() {
    DrawText("--- SHIP UPGRADES ---", 10, 94, 16, LIGHTGRAY);
    GameState& s = GameState::Get();
    ShipConfig& c = s.ship_config;

    struct Upg { const char* name; const char* desc; int cost; bool* flag; };
    Upg upgrades[] = {
        {"Hull Plating I",    "+50 max HP",             300,  &c.hull1},
        {"Hull Plating II",   "+100 max HP",            800,  &c.hull2},
        {"Cargo Hold I",      "+50 cargo capacity",     250,  &c.cargo1},
        {"Cargo Hold II",     "+100 cargo capacity",    600,  &c.cargo2},
        {"Engine Boost",      "+100 speed & thrust",    400,  &c.engine_boost},
        {"FTL Capacitor",     "FTL charge time -2s",    500,  &c.ftl_cap},
        {"Better Optics",     "Flashlight range +100",  350,  &c.better_optics},
        {"Extended Mag",      "+50% ammo all weapons",  300,  &c.extended_mag},
        {"Fast Loader",       "-20% cooldown all",      350,  &c.fast_loader},
        {"Penetrator Rounds", "+1 pen all projectiles", 450,  &c.pen_rounds},
        {"Drone Pack",        "+5 drones/mission",      200,  &c.drone_pack},
        {"Flare Pack",        "+8 flares/mission",      150,  &c.flare_pack},
        {"Catcher Capacity",  "10 tether slots",        250,  &c.catcher_cap},
    };
    static const int N = 13;

    int y = 120;
    for (int i = 0; i < N; i++) {
        Upg& u = upgrades[i];
        bool bought = *u.flag;
        Color tc = bought ? DARKGRAY : WHITE;
        DrawText(u.name, 10, y, 14, tc);
        DrawText(u.desc, 210, y, 12, GRAY);

        if (bought) {
            DrawText("[PURCHASED]", 500, y, 12, DARKGREEN);
        } else {
            DrawText(TextFormat("%d cr", u.cost), 500, y, 12, GOLD);
            if (Button(570, y-2, 70, 20, "BUY", {40,60,40,220}, WHITE)) {
                if (s.money >= u.cost) {
                    s.money -= u.cost;
                    *u.flag = true;
                    // Apply effect
                    if (&c.hull1 == u.flag)          c.max_hp           += 50;
                    if (&c.hull2 == u.flag)          c.max_hp           += 100;
                    if (&c.cargo1 == u.flag)         c.max_cargo        += 50;
                    if (&c.cargo2 == u.flag)         c.max_cargo        += 100;
                    if (&c.engine_boost == u.flag)   { c.max_speed += 100; c.thrust += 100; }
                    if (&c.ftl_cap == u.flag)        c.ftl_charge_time  -= 2.f;
                    if (&c.better_optics == u.flag)  c.flashlight_range += 100.f;
                    if (&c.extended_mag == u.flag)   c.ammo_multiplier   = 1.5f;
                    if (&c.fast_loader == u.flag)    c.cooldown_mult     = 0.8f;
                    if (&c.pen_rounds == u.flag)     c.pen_bonus         = 1;
                    if (&c.drone_pack == u.flag)     c.drone_count       += 5;
                    if (&c.flare_pack == u.flag)     c.flare_count       += 8;
                    if (&c.catcher_cap == u.flag)    c.catcher_capacity  = 10;
                    SetStatus(TextFormat("Purchased: %s", u.name));
                    SaveSystem::Save(s);
                } else {
                    SetStatus("Insufficient funds");
                }
            }
        }
        y += 26;
    }
}

void BaseScene::DrawLaunchButton() {
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    if (Button(sw - 220, sh - 50, 210, 40, "LAUNCH MISSION", {30,80,30,240}, WHITE)) {
        SaveSystem::Save(GameState::Get());
        game->StartMission();
    }
}

void BaseScene::Shutdown() {}
