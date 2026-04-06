#include "weapon_manager.h"
#include "mission/asteroid.h"
#include "mission/chunk.h"
#include "raymath.h"
#include <cmath>
#include <algorithm>

void WeaponManager::Init(const ShipConfig& cfg) {
    ammo_mult   = cfg.ammo_multiplier;
    cd_mult     = cfg.cooldown_mult;
    pen_bonus   = cfg.pen_bonus;
    catcher_cap = cfg.catcher_capacity;

    auto ammo = [&](int base) -> int {
        return base < 0 ? -1 : (int)(base * ammo_mult);
    };

    //       ammo             cooldown
    slots[0] = { -1,           0.20f * cd_mult }; // Regular
    slots[1] = { ammo(60),     0.40f * cd_mult }; // AP
    slots[2] = { ammo(30),     0.60f * cd_mult }; // Shrapnel
    slots[3] = { ammo(20),     1.00f * cd_mult }; // Explosive
    slots[4] = { ammo(15),     1.50f * cd_mult }; // Cluster
    slots[5] = { ammo(10),     2.00f * cd_mult }; // Timed mine
    slots[6] = { -1,           0.30f * cd_mult }; // Catcher
    slots[7] = { ammo(cfg.flare_count), 1.00f * cd_mult }; // Flare
    slots[8] = { ammo(cfg.drone_count), 3.00f * cd_mult }; // Intel drone

    // Clamp cooldown floors
    for (auto& s : slots) {
        s.cooldown      = fmaxf(0.05f, s.cooldown);
        s.cooldown_left = 0.f;
    }
}

void WeaponManager::HandleInput(Vector2 ship_pos_val, Camera2D& cam) {
    // Weapon slot switching
    for (int i = 0; i < 9; i++)
        if (IsKeyPressed(KEY_ONE + i)) active_slot = i;

    // Right-click: detonate all placed mines
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        for (auto& p : projectiles) {
            if (auto* mine = dynamic_cast<TimedMine*>(p.get()))
                mine->detonate_now = true;
        }
    }

    // Left-click or hold: fire
    bool fire_now = false;
    if (active_slot == 0 || active_slot == 1 || active_slot == 6) {
        fire_now = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    } else {
        fire_now = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    }

    if (fire_now && slots[active_slot].CanFire()) {
        Vector2 mouse_world = GetScreenToWorld2D(GetMousePosition(), cam);
        Vector2 dir = Vector2Normalize(Vector2Subtract(mouse_world, ship_pos_val));
        Fire(active_slot, ship_pos_val, dir);
    }
}

void WeaponManager::Fire(int slot_idx, Vector2 origin, Vector2 direction) {
    WeaponSlot& slot = slots[slot_idx];
    if (!slot.CanFire()) return;
    slot.cooldown_left = slot.cooldown;
    if (slot.ammo > 0) slot.ammo--;

    switch (slot_idx) {
        case 0: { // Regular
            auto p     = std::make_unique<Projectile>();
            p->pos     = origin;
            p->vel     = Vector2Scale(direction, 600.f);
            p->penetration = 1 + pen_bonus;
            p->damage  = 15;
            p->lifetime = 3.f;
            projectiles.push_back(std::move(p));
            break;
        }
        case 1: { // AP
            auto p     = std::make_unique<Projectile>();
            p->pos     = origin;
            p->vel     = Vector2Scale(direction, 800.f);
            p->penetration = 4 + pen_bonus;
            p->damage  = 10;
            p->lifetime = 2.f;
            projectiles.push_back(std::move(p));
            break;
        }
        case 2: { // Shrapnel
            auto p = std::make_unique<ShrapnelBullet>();
            p->pos     = origin;
            p->vel     = Vector2Scale(direction, 400.f);
            p->penetration = 1 + pen_bonus;
            p->damage  = 5;
            p->lifetime = 4.f;
            p->frag_count = 6;
            p->spawn_fragment = [this](Vector2 pos, Vector2 vel) {
                SpawnFragment(pos, vel);
            };
            projectiles.push_back(std::move(p));
            break;
        }
        case 3: { // Explosive shell
            auto p = std::make_unique<ExplosiveShell>();
            p->pos     = origin;
            p->vel     = Vector2Scale(direction, 400.f);
            p->penetration = 2 + pen_bonus;
            p->damage  = 20;
            p->lifetime = 4.f;
            p->exp_radius = 80.f;
            p->exp_damage = 40;
            p->explosions = &explosions;
            projectiles.push_back(std::move(p));
            break;
        }
        case 4: { // Cluster bomb
            auto p = std::make_unique<ClusterBomb>();
            p->pos     = origin;
            p->vel     = Vector2Scale(direction, 300.f);
            p->penetration = 1 + pen_bonus;
            p->damage  = 5;
            p->lifetime = 4.f;
            p->explosions = &explosions;
            p->spawn_sub = [this](Vector2 pos, Vector2 vel, float er, int ed) {
                SpawnSubMunition(pos, vel, er, ed);
            };
            projectiles.push_back(std::move(p));
            break;
        }
        case 5: { // Timed mine
            auto p = std::make_unique<TimedMine>();
            p->pos     = origin;
            p->vel     = Vector2Scale(direction, 150.f);
            p->penetration = 3 + pen_bonus;
            p->damage  = 5;
            p->lifetime = 6.f;
            p->fuse_time = 2.f;
            p->exp_radius = 120.f;
            p->exp_damage = 60;
            p->explosions = &explosions;
            projectiles.push_back(std::move(p));
            break;
        }
        case 6: { // Catcher
            auto p = std::make_unique<CatcherProjectile>();
            p->pos     = origin;
            p->vel     = Vector2Scale(direction, 350.f);
            p->penetration = 0;
            p->damage  = 0;
            p->lifetime = 2.f;
            p->max_tethers = catcher_cap;
            if (ship_pos) p->ship_pos = *ship_pos;
            projectiles.push_back(std::move(p));
            break;
        }
        case 7: { // Flare
            auto p = std::make_unique<FlareProjectile>();
            p->pos     = origin;
            p->vel     = Vector2Scale(direction, 200.f);
            p->penetration = 0;
            p->damage  = 0;
            p->lifetime = 3.f;
            projectiles.push_back(std::move(p));
            break;
        }
        case 8: { // Intel drone
            auto p = std::make_unique<IntelDrone>();
            p->pos     = origin;
            p->vel     = Vector2Scale(direction, 200.f);
            p->penetration = 0;
            p->damage  = 0;
            p->lifetime = 8.f;
            p->report  = &scan_report;
            projectiles.push_back(std::move(p));
            break;
        }
    }
}

void WeaponManager::SpawnFragment(Vector2 pos, Vector2 vel) {
    auto p = std::make_unique<Projectile>();
    p->pos     = pos;
    p->vel     = vel;
    p->penetration = 0 + pen_bonus;
    p->damage  = 8;
    p->lifetime = 1.f;
    projectiles.push_back(std::move(p));
}

void WeaponManager::SpawnSubMunition(Vector2 pos, Vector2 vel, float er, int ed) {
    auto p = std::make_unique<ExplosiveShell>();
    p->pos     = pos;
    p->vel     = vel;
    p->penetration = 1 + pen_bonus;
    p->damage  = 5;
    p->lifetime = 2.f;
    p->exp_radius = er;
    p->exp_damage = ed;
    p->explosions = &explosions;
    projectiles.push_back(std::move(p));
}

void WeaponManager::Update(float dt) {
    // Cooldown ticks
    for (auto& s : slots)
        if (s.cooldown_left > 0) s.cooldown_left -= dt;

    // Update catcher ship_pos reference
    if (ship_pos) {
        for (auto& p : projectiles) {
            if (auto* c = dynamic_cast<CatcherProjectile*>(p.get()))
                c->ship_pos = *ship_pos;
        }
    }

    // Update projectiles
    if (asteroids && chunks) {
        for (auto& p : projectiles)
            p->Update(dt, *asteroids, *chunks);
    }

    // Remove dead projectiles
    projectiles.erase(
        std::remove_if(projectiles.begin(), projectiles.end(),
                       [](const std::unique_ptr<Projectile>& p){ return !p->alive; }),
        projectiles.end());

    // Update explosion visuals
    for (auto& e : explosions) e.timer -= dt;
    explosions.erase(
        std::remove_if(explosions.begin(), explosions.end(),
                       [](const Explosion& e){ return e.timer <= 0.f; }),
        explosions.end());

    // Scan report timer
    if (scan_report.has_data && scan_report.display_timer > 0)
        scan_report.display_timer -= dt;
}

void WeaponManager::Draw() const {
    for (auto& p : projectiles)
        p->Draw();
    for (auto& e : explosions)
        e.Draw();
}

void WeaponManager::DrawHUD() const {
    static const char* names[9] = {
        "REG","AP","SHRAP","EXP","CLUST","MINE","CATCH","FLARE","DRONE"
    };

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int slot_w = 70, slot_h = 46, spacing = 4;
    int total_w = 9 * slot_w + 8 * spacing;
    int sx = (sw - total_w) / 2;
    int sy = sh - slot_h - 10;

    for (int i = 0; i < 9; i++) {
        int x = sx + i * (slot_w + spacing);
        Color bg  = (i == active_slot) ? Color{60, 60, 80, 220} : Color{30, 30, 40, 180};
        Color bdr = (i == active_slot) ? WHITE : GRAY;
        DrawRectangle(x, sy, slot_w, slot_h, bg);
        DrawRectangleLines(x, sy, slot_w, slot_h, bdr);
        DrawText(TextFormat("%d", i+1), x+3, sy+3, 11, LIGHTGRAY);
        DrawText(names[i], x+3, sy+16, 10, WHITE);

        const WeaponSlot& s = slots[i];
        if (s.ammo == -1) {
            DrawText("∞", x+3, sy+30, 12, SKYBLUE);
        } else if (s.ammo == 0) {
            DrawText("EMPTY", x+3, sy+30, 10, RED);
        } else {
            DrawText(TextFormat("%d", s.ammo), x+3, sy+30, 12,
                     s.ammo <= 3 ? ORANGE : LIGHTGRAY);
        }

        // Cooldown bar
        if (s.cooldown_left > 0) {
            float frac = s.cooldown_left / s.cooldown;
            DrawRectangle(x, sy + slot_h - 4, (int)((1.f - frac) * slot_w), 4, GREEN);
        } else {
            DrawRectangle(x, sy + slot_h - 4, slot_w, 4, GREEN);
        }
    }

    // Scan report popup
    if (scan_report.has_data && scan_report.display_timer > 0) {
        static const char* mat_names[] = {"","Rock","Iron","Copper","Silicon","Titanium","Gold"};
        int py = 250;
        DrawText("[DRONE SCAN]", sw - 210, py, 14, SKYBLUE); py += 18;
        for (auto& [mat, cnt] : scan_report.tile_counts) {
            int m = (int)mat;
            if (m >= 0 && m < 7 && cnt > 0) {
                DrawText(TextFormat("  %-9s %d", mat_names[m], cnt),
                         sw - 210, py, 13, WHITE);
                py += 16;
            }
        }
    }
}
