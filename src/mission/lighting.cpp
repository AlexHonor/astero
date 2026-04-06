#include "lighting.h"
#include <cmath>

void LightingSystem::Init(int screen_w, int screen_h) {
    sw = screen_w;
    sh = screen_h;
    mask = LoadRenderTexture(sw, sh);

    // Bake radial gradient (128x128, white center → transparent edge)
    Image img = GenImageColor(128, 128, {0, 0, 0, 0});
    for (int y = 0; y < 128; y++) {
        for (int x = 0; x < 128; x++) {
            float dx = (x - 64) / 64.f;
            float dy = (y - 64) / 64.f;
            float dist = sqrtf(dx*dx + dy*dy);
            float alpha = fmaxf(0.f, 1.f - dist);
            alpha = alpha * alpha;   // quadratic falloff
            ImageDrawPixel(&img, x, y,
                {255, 255, 255, (unsigned char)(alpha * 255)});
        }
    }
    gradient = LoadTextureFromImage(img);
    UnloadImage(img);
}

void LightingSystem::Shutdown() {
    UnloadRenderTexture(mask);
    UnloadTexture(gradient);
}

int LightingSystem::AddLight(Vector2 pos, float radius, Color color,
                              float lifetime, bool flicker) {
    int id = next_id++;
    lights[id] = {pos, radius, color, lifetime, flicker, true};
    return id;
}

void LightingSystem::MoveLight(int id, Vector2 pos) {
    auto it = lights.find(id);
    if (it != lights.end()) it->second.pos = pos;
}

void LightingSystem::RemoveLight(int id) {
    lights.erase(id);
}

void LightingSystem::Update(float dt) {
    for (auto& [id, l] : lights) {
        if (!l.alive) continue;
        if (l.lifetime > 0.f) {
            l.lifetime -= dt;
            if (l.lifetime <= 0.f) l.alive = false;
        }
    }
    // Remove dead lights
    for (auto it = lights.begin(); it != lights.end(); ) {
        if (!it->second.alive) it = lights.erase(it);
        else ++it;
    }
}

void LightingSystem::BakeMask(Camera2D& cam) {
    BeginTextureMode(mask);
    ClearBackground(BLACK);   // fully dark

    BeginMode2D(cam);
    BeginBlendMode(BLEND_ADDITIVE);

    for (auto& [id, l] : lights) {
        if (!l.alive) continue;
        float r = l.radius;
        if (l.flicker)
            r += sinf(GetTime() * 7.f + id * 1.3f) * r * 0.05f;

        Rectangle src = {0, 0, 128, 128};
        Rectangle dst = {l.pos.x - r, l.pos.y - r, r * 2.f, r * 2.f};
        DrawTexturePro(gradient, src, dst, {0, 0}, 0.f, l.color);
    }

    EndBlendMode();
    EndMode2D();
    EndTextureMode();
}

void LightingSystem::Draw(Camera2D& cam) {
    BakeMask(cam);

    BeginBlendMode(BLEND_MULTIPLIED);
    // Flip Y for render texture
    DrawTextureRec(mask.texture,
        {0, 0, (float)sw, -(float)sh},
        {0, 0}, WHITE);
    EndBlendMode();
}
