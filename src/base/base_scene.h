#pragma once
#include "raylib.h"
#include "core/game_state.h"
#include "base/market.h"
#include "base/quests.h"
#include "base/crafting.h"
#include <string>

class Game;

enum class BaseTab { Inventory, Market, Quests, Crafting, Ship };

class BaseScene {
public:
    void Init(Game* game, const MissionResult& result);
    void Update(float dt);
    void Draw();
    void Shutdown();

private:
    Game*         game = nullptr;
    MissionResult last_result;
    BaseTab       tab  = BaseTab::Inventory;

    Market        market;
    QuestManager  quests;
    Crafting      crafting;

    // Market panel state
    int  market_qty[11] = {};   // qty to trade per resource
    std::string status_msg;
    float status_timer = 0.f;

    void DrawTabBar();
    void DrawInventory();
    void DrawMarket();
    void DrawQuests();
    void DrawCrafting();
    void DrawShipUpgrades();
    void DrawLaunchButton();

    void SetStatus(const char* msg) { status_msg = msg; status_timer = 3.f; }

    // Simple button helper: returns true if clicked
    bool Button(int x, int y, int w, int h, const char* label, Color bg, Color fg);
};
