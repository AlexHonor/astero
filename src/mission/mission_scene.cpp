#include "mission_scene.h"
#include "core/game.h"
#include "mission/asteroid_generator.h"

void MissionScene::Init(Game* g, const ShipConfig& cfg) {
    game = g;
    (void)cfg;

    // Camera centered in world
    cam.target   = {2000.f, 2000.f};
    cam.offset   = {GetScreenWidth() / 2.f, GetScreenHeight() / 2.f};
    cam.rotation = 0.f;
    cam.zoom     = 1.f;

    // Generate asteroid field
    FieldConfig field_cfg;
    field_cfg.num_asteroids = 12;
    AsteroidGenerator::Generate(asteroids, field_cfg, 42);

    // Wire up spawn callbacks
    for (auto& ast : asteroids) {
        ast.on_spawn = [this](Vector2 pos, TileMat mat, Vector2 imp, bool is_chunk) {
            SpawnCallback(pos, mat, imp, is_chunk);
        };
    }
}

void MissionScene::SpawnCallback(Vector2 pos, TileMat mat, Vector2 impulse, bool is_chunk) {
    if (is_chunk) {
        Chunk c;
        c.pos      = pos;
        c.vel      = impulse;
        c.material = mat;
        chunks.push_back(c);
    }
    // Dust: will be wired in Phase 7
}

void MissionScene::Update(float dt) {
    // Scroll zoom
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        cam.zoom = fmaxf(0.3f, fminf(3.f, cam.zoom + wheel * 0.1f));
    }

    // Pan camera with arrow keys for now (ship movement comes in Phase 3)
    float pan = 300.f * dt / cam.zoom;
    if (IsKeyDown(KEY_LEFT))  cam.target.x -= pan;
    if (IsKeyDown(KEY_RIGHT)) cam.target.x += pan;
    if (IsKeyDown(KEY_UP))    cam.target.y -= pan;
    if (IsKeyDown(KEY_DOWN))  cam.target.y += pan;

    // Update asteroids
    for (auto& ast : asteroids)
        ast.Update(dt);

    // Remove dead asteroids
    asteroids.erase(
        std::remove_if(asteroids.begin(), asteroids.end(),
                       [](const Asteroid& a){ return !a.IsAlive(); }),
        asteroids.end());

    // Update chunks
    for (auto& c : chunks) c.Update(dt);
    chunks.erase(
        std::remove_if(chunks.begin(), chunks.end(),
                       [](const Chunk& c){ return !c.alive; }),
        chunks.end());

    // Return to base on ESC
    if (IsKeyPressed(KEY_ESCAPE)) {
        MissionResult result;
        result.player_died = false;
        game->EndMission(result);
    }
}

void MissionScene::Draw() {
    BeginMode2D(cam);

    for (auto& ast : asteroids)
        ast.Draw();

    for (auto& c : chunks)
        c.Draw();

    EndMode2D();

    // HUD
    DrawText("MISSION MODE", 10, 10, 18, WHITE);
    DrawText("Arrows: pan | Scroll: zoom | ESC: return to base", 10, 35, 14, GRAY);
    DrawText(TextFormat("Asteroids: %d  Chunks: %d",
             (int)asteroids.size(), (int)chunks.size()), 10, 55, 14, LIGHTGRAY);
}

void MissionScene::Shutdown() {}
