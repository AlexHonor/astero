#pragma once
#include "raylib.h"
#include <unordered_map>

struct LightSource {
    Vector2 pos;
    float   radius;
    Color   color;
    float   lifetime   = -1.f;   // -1 = permanent
    bool    flicker    = false;
    bool    alive      = true;
};

class LightingSystem {
public:
    void Init(int screen_w, int screen_h);
    void Shutdown();

    int  AddLight(Vector2 pos, float radius, Color color,
                  float lifetime = -1.f, bool flicker = false);
    void MoveLight(int id, Vector2 pos);
    void RemoveLight(int id);

    void Update(float dt);
    // Call after EndMode2D — blits the light mask over the screen
    void Draw(Camera2D& cam);

private:
    RenderTexture2D   mask      = {0};
    Texture2D         gradient  = {0};
    std::unordered_map<int, LightSource> lights;
    int next_id = 0;
    int sw = 0, sh = 0;

    void BakeMask(Camera2D& cam);
};
