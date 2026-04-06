#include "mission_scene.h"
#include "core/game.h"
#include "mission/asteroid_generator.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>

static constexpr float WORLD_SIZE        = 4000.f;
static constexpr float COLLECT_RADIUS    = 24.f;
static constexpr float BOUNDARY_DAMAGE   = 5.f;   // hp/s outside world

void MissionScene::Init(Game* g, const ShipConfig& cfg) {
    game = g;
    mission_timer      = 0.f;
    boundary_dmg_timer = 0.f;

    // Camera
    cam.target   = {WORLD_SIZE / 2, WORLD_SIZE / 2};
    cam.offset   = {GetScreenWidth() / 2.f, GetScreenHeight() / 2.f};
    cam.rotation = 0.f;
    cam.zoom     = 1.f;

    // Ship
    ship.Init(cfg, {200.f, WORLD_SIZE / 2.f});
    ftl.Init(cfg.ftl_charge_time);

    // Weapons
    weapons.Init(cfg);
    weapons.asteroids = &asteroids;
    weapons.chunks    = &chunks;
    weapons.ship_pos  = &ship.pos;

    // Generate asteroid field
    FieldConfig field_cfg;
    field_cfg.num_asteroids = 12;
    field_cfg.deep_mat_bias = 0.1f * (GameState::Get().run_number - 1);
    AsteroidGenerator::Generate(asteroids, field_cfg, (uint32_t)GetTime() ^ 0x12345678);
    RewireSpawnCallbacks();

    chunks.clear();
    minimap.Init();
    dust.Init(GetScreenWidth(), GetScreenHeight());
    lighting.Init(GetScreenWidth(), GetScreenHeight());
    ship_light_id    = lighting.AddLight(ship.pos, cfg.flashlight_range,
                                         {255, 240, 200, 200});
    ambient_light_id = lighting.AddLight(ship.pos, 80.f,
                                         {180, 180, 220, 80});
}

void MissionScene::RewireSpawnCallbacks() {
    for (auto& ast : asteroids) {
        ast.on_spawn = [this](Vector2 pos, TileMat mat, Vector2 imp, bool is_chunk) {
            SpawnCallback(pos, mat, imp, is_chunk);
        };
    }
}

void MissionScene::SpawnCallback(Vector2 pos, TileMat mat, Vector2 impulse, bool is_chunk) {
    if (is_chunk && mat != TileMat::Rock) {
        Chunk c;
        c.pos      = pos;
        c.vel      = impulse;
        c.material = mat;
        chunks.push_back(c);
    } else {
        dust.AddDust(pos, mat, impulse);
    }
}

void MissionScene::Update(float dt) {
    mission_timer += dt;

    // Scroll zoom
    float wheel = GetMouseWheelMove();
    if (wheel != 0)
        cam.zoom = fmaxf(0.3f, fminf(3.f, cam.zoom + wheel * 0.1f));

    // Update ship
    ship.Update(dt, cam);
    ship.UpdateFlash(dt);
    weapons.HandleInput(ship.pos, cam);
    weapons.Update(dt);

    // Screen shake from nearby explosions
    for (auto& e : weapons.explosions) {
        if (e.timer > 0.38f) {  // first tick
            float dist = Vector2Distance(e.pos, ship.pos);
            if (dist < e.radius * 3.f) {
                float strength = 1.f - (dist / (e.radius * 3.f));
                shake_trauma = fminf(1.f, shake_trauma + strength * 0.4f);
            }
        }
    }
    if (shake_trauma > 0.f) {
        float shake_amt = shake_trauma * shake_trauma * 12.f;
        cam.offset.x = GetScreenWidth()  / 2.f + ((float)rand()/RAND_MAX - 0.5f) * shake_amt;
        cam.offset.y = GetScreenHeight() / 2.f + ((float)rand()/RAND_MAX - 0.5f) * shake_amt;
        shake_trauma -= dt * 3.f;
        if (shake_trauma < 0.f) {
            shake_trauma = 0.f;
            cam.offset   = {GetScreenWidth() / 2.f, GetScreenHeight() / 2.f};
        }
    }

    // Explosion dust
    for (auto& e : weapons.explosions)
        if (e.timer > 0.38f)  // only on the first tick
            dust.AddExplosionDust(e.pos, e.radius, TileMat::Rock);

    // Move ship lights
    lighting.MoveLight(ship_light_id, ship.pos);
    lighting.MoveLight(ambient_light_id, ship.pos);

    // Add flare lights from newly-landed flares
    for (auto& p : weapons.projectiles) {
        if (auto* f = dynamic_cast<FlareProjectile*>(p.get())) {
            if (f->landed && f->light_id == -1) {
                f->light_id = lighting.AddLight(
                    f->pos, 200.f, {255, 200, 130, 220}, f->light_life, true);
            }
        }
    }
    lighting.Update(dt);
    dust.Update(dt);

    // Smooth camera follow
    cam.target.x += (ship.pos.x - cam.target.x) * 5.f * dt;
    cam.target.y += (ship.pos.y - cam.target.y) * 5.f * dt;

    // Update asteroids (may grow via splits)
    for (int i = 0; i < (int)asteroids.size(); i++) {
        asteroids[i].Update(dt);
        asteroids[i].CheckSplit(asteroids);
    }
    asteroids.erase(
        std::remove_if(asteroids.begin(), asteroids.end(),
                       [](const Asteroid& a){ return !a.IsAlive(); }),
        asteroids.end());
    RewireSpawnCallbacks();  // re-wire after potential removals/additions

    // Update chunks
    for (auto& c : chunks) c.Update(dt);
    chunks.erase(
        std::remove_if(chunks.begin(), chunks.end(),
                       [](const Chunk& c){ return !c.alive; }),
        chunks.end());

    CheckShipAsteroidCollision();
    CollectNearbyChunks();

    // Boundary damage
    if (ship.pos.x < 0 || ship.pos.x > WORLD_SIZE ||
        ship.pos.y < 0 || ship.pos.y > WORLD_SIZE) {
        boundary_dmg_timer += dt;
        if (boundary_dmg_timer >= 1.f) {
            ship.TakeDamage((int)BOUNDARY_DAMAGE);
            boundary_dmg_timer = 0.f;
        }
    }

    // FTL
    if (IsKeyPressed(KEY_F) && !ftl.charging && !ftl.on_cooldown)
        ftl.TryActivate();

    // Lock ship movement while charging
    if (ftl.charging) {
        ship.vel = {0, 0};
    }

    ftl.Update(dt);

    // Death / escape
    if (!ship.alive) {
        MissionResult result;
        result.player_died = true;
        game->EndMission(result);
        return;
    }
    if (ftl.IsComplete()) {
        MissionResult result;
        result.player_died      = false;
        result.ship_hp_fraction = ship.hp / (float)ship.max_hp;
        game->EndMission(result);
        return;
    }
}

void MissionScene::CheckShipAsteroidCollision() {
    Rectangle ship_box = ship.Bounds();

    for (auto& ast : asteroids) {
        for (int r = 0; r < ast.rows; r++) {
            for (int c = 0; c < ast.cols; c++) {
                if (ast.cells[r][c].material == TileMat::None) continue;
                Vector2 tp = ast.TileWorldPos(c, r);
                float half = TILE_SIZE * 0.5f;
                Rectangle tile_box = {tp.x - half, tp.y - half, TILE_SIZE, TILE_SIZE};

                if (CheckCollisionRecs(ship_box, tile_box)) {
                    float impact = Vector2Length(Vector2Subtract(ship.vel, ast.velocity));
                    if (impact > 20.f) {
                        ship.TakeDamage((int)(impact * 0.1f));
                        ftl.Interrupt();
                    }
                    // Push ship away
                    Vector2 push = Vector2Normalize(Vector2Subtract(ship.pos, tp));
                    ship.vel = Vector2Add(
                        Vector2Scale(ship.vel, -0.3f),
                        Vector2Scale(push, 40.f));
                    return;
                }
            }
        }
    }
}

void MissionScene::CollectNearbyChunks() {
    for (auto& ch : chunks) {
        if (!ch.alive) continue;
        if (ship.cargo >= ship.max_cargo) break;
        if (Vector2Distance(ch.pos, ship.pos) < COLLECT_RADIUS) {
            GameState::Get().AddResource(
                static_cast<ResourceType>(ch.material), 1);
            ship.cargo++;
            ch.alive = false;
        }
    }
}

void MissionScene::DrawHUD() const {
    int sw = GetScreenWidth();

    // HP bar
    DrawRectangle(10, 10, 200, 16, DARKGRAY);
    float hp_frac = ship.max_hp > 0 ? ship.hp / (float)ship.max_hp : 0.f;
    Color hp_col  = hp_frac > 0.5f ? GREEN : (hp_frac > 0.25f ? YELLOW : RED);
    DrawRectangle(10, 10, (int)(200 * hp_frac), 16, hp_col);
    DrawRectangleLines(10, 10, 200, 16, WHITE);
    DrawText(TextFormat("HP %d/%d", ship.hp, ship.max_hp), 14, 12, 12, WHITE);

    // Cargo bar
    DrawRectangle(10, 32, 200, 16, DARKGRAY);
    float cargo_frac = ship.max_cargo > 0 ? ship.cargo / (float)ship.max_cargo : 0.f;
    DrawRectangle(10, 32, (int)(200 * cargo_frac), 16, SKYBLUE);
    DrawRectangleLines(10, 32, 200, 16, WHITE);
    DrawText(TextFormat("Cargo %d/%d", ship.cargo, ship.max_cargo), 14, 34, 12, WHITE);

    // Timer
    int mins = (int)(mission_timer / 60);
    int secs = (int)(mission_timer) % 60;
    DrawText(TextFormat("%02d:%02d", mins, secs), sw / 2 - 24, 10, 20, WHITE);

    // FTL system (bottom right)
    int ftl_x = sw - 160, ftl_y = GetScreenHeight() - 80;
    DrawRectangle(ftl_x, ftl_y, 150, 30, DARKGRAY);
    if (ftl.charging) {
        int fill = (int)(ftl.charge_progress * 150);
        DrawRectangle(ftl_x, ftl_y, fill, 30, SKYBLUE);
        DrawText("FTL CHARGING...", ftl_x + 4, ftl_y + 8, 12, BLACK);
    } else if (ftl.on_cooldown) {
        DrawRectangle(ftl_x, ftl_y, 150, 30, Color{60, 30, 30, 255});
        DrawText(TextFormat("FTL COOLDOWN %.0fs", ftl.cooldown_left),
                 ftl_x + 4, ftl_y + 8, 12, RED);
    } else {
        DrawRectangle(ftl_x, ftl_y, 150, 30, Color{20, 60, 60, 255});
        DrawText("F  - FTL ESCAPE", ftl_x + 4, ftl_y + 8, 12, SKYBLUE);
    }
    DrawRectangleLines(ftl_x, ftl_y, 150, 30, WHITE);

    // Controls hint
    DrawText("WASD: move | 1-9: weapon | ESC: abort", 10, GetScreenHeight() - 24, 13, DARKGRAY);

    // Boundary warning
    bool near_boundary =
        ship.pos.x < 100 || ship.pos.x > WORLD_SIZE - 100 ||
        ship.pos.y < 100 || ship.pos.y > WORLD_SIZE - 100;
    if (near_boundary && ((int)(mission_timer * 4) % 2 == 0)) {
        DrawRectangleLinesEx({2, 2, (float)sw - 4, (float)GetScreenHeight() - 4}, 4, RED);
        DrawText("BOUNDARY WARNING", sw / 2 - 90, 36, 16, RED);
    }
}

void MissionScene::Draw() {
    BeginMode2D(cam);

    for (auto& ast : asteroids)
        ast.Draw();
    for (auto& c : chunks)
        c.Draw();
    ship.Draw();
    weapons.Draw();

    EndMode2D();

    dust.Draw(cam);
    lighting.Draw(cam);

    minimap.Update(asteroids, chunks, ship);
    DrawHUD();
    weapons.DrawHUD();
    minimap.Draw(GetScreenWidth() - Minimap::MAP_SIZE - 10, 10);
}

void MissionScene::Shutdown() {
    minimap.Shutdown();
    lighting.Shutdown();
    dust.Shutdown();
}
