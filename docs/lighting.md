# 2D Lighting System

## Approach

raylib has no built-in 2D lighting. We implement it with a `RenderTexture2D` **light mask**:

1. Render the light mask texture entirely black (darkness).
2. Draw each light source as a white/colored radial gradient circle onto the mask using `BLEND_ADDITIVE`.
3. Draw the final game scene normally.
4. Blit the light mask over the scene using `BLEND_MULTIPLIED` — dark areas of the mask darken the scene, bright areas let it through.

This gives a clean, cheap darkness effect with multiple light sources.

---

## LightingSystem

```cpp
struct Light {
    Vector2 pos;
    float   radius;
    Color   color;       // tint of the light
    float   lifetime;    // -1 = permanent, >0 = burns out
    bool    flicker;     // if true, radius oscillates slightly
    bool    alive = true;
};

class LightingSystem {
public:
    void Init(int screen_w, int screen_h);
    void Shutdown();

    int  AddLight(Vector2 pos, float radius, Color color,
                  float lifetime = -1.f, bool flicker = false);
    void MoveLight(int id, Vector2 pos);   // for ship flashlight
    void RemoveLight(int id);

    void Update(float dt);
    void Draw(Camera2D& cam);  // builds mask, blits to screen

private:
    RenderTexture2D mask;
    Texture2D       radial_gradient;  // pre-baked white→transparent circle
    std::unordered_map<int, Light> lights;
    int next_id = 0;

    void BakeMask(Camera2D& cam);
};
```

---

## Pre-baked Radial Gradient Texture

Generated once at startup (128×128 px, white center → transparent edge):

```cpp
Image img = GenImageColor(128, 128, {0,0,0,0});
for (int y = 0; y < 128; y++) {
    for (int x = 0; x < 128; x++) {
        float dx = (x - 64) / 64.f;
        float dy = (y - 64) / 64.f;
        float dist = sqrtf(dx*dx + dy*dy);
        float alpha = fmaxf(0.f, 1.f - dist);
        alpha = alpha * alpha;   // quadratic falloff — softer edges
        ImageDrawPixel(&img, x, y, {255, 255, 255, (unsigned char)(alpha * 255)});
    }
}
radial_gradient = LoadTextureFromImage(img);
UnloadImage(img);
```

---

## Building the Light Mask

```cpp
void LightingSystem::BakeMask(Camera2D& cam) {
    BeginTextureMode(mask);
    ClearBackground(BLACK);         // start fully dark

    BeginMode2D(cam);
    BeginBlendMode(BLEND_ADDITIVE); // lights add brightness

    for (auto& [id, light] : lights) {
        if (!light.alive) continue;
        float r = light.radius;
        if (light.flicker)
            r += sinf(GetTime() * 7.f + id) * r * 0.05f;  // subtle flicker

        // Scale gradient texture to light radius * 2, center on light pos
        Rectangle src = {0, 0, 128, 128};
        Rectangle dst = {light.pos.x - r, light.pos.y - r, r*2, r*2};
        DrawTexturePro(radial_gradient, src, dst, {0,0}, 0.f, light.color);
    }

    EndBlendMode();
    EndMode2D();
    EndTextureMode();
}
```

---

## Applying the Mask to Screen

```cpp
// In LightingSystem::Draw(), called after the main scene is drawn:
BakeMask(cam);

BeginBlendMode(BLEND_MULTIPLIED);
DrawTextureRec(mask.texture,
    {0, 0, (float)mask.texture.width, -(float)mask.texture.height},
    {0, 0}, WHITE);
EndBlendMode();
```

`BLEND_MULTIPLIED`: `result = screen_pixel * mask_pixel`. Black mask = black screen. White mask = unchanged screen. This is the darkness effect.

---

## Ship Flashlight

A permanent light (id stored in `ship_light_id`) that moves with the ship each frame:

```cpp
// On ship update:
Vector2 mouse_world = GetScreenToWorld2D(GetMousePosition(), cam);
Vector2 dir = Vector2Normalize(Vector2Subtract(mouse_world, ship.pos));
// Offset light slightly forward (not centered on ship)
Vector2 light_pos = Vector2Add(ship.pos, Vector2Scale(dir, 20.f));
lighting.MoveLight(ship_light_id, light_pos);
```

The flashlight is a wide, elongated light. Approximate with two overlapping circles:
- Main: radius 350, color `{255, 240, 200, 200}`
- Small ambient: radius 80 centered on ship, color `{180, 180, 220, 80}` (dim blue ambient)

A true cone shape can be achieved later with a custom cone gradient texture.

---

## Flare Light

Added when flare lands, auto-removes on expiry:

```cpp
int flare_id = lighting.AddLight(pos, 200.f, {255, 200, 130, 220},
                                  15.f,  // 15 second lifetime
                                  true); // flicker enabled
```

`LightingSystem::Update()` decrements `lifetime` and calls `RemoveLight()` when expired.

---

## Darkness Level

`CanvasModulate` equivalent: the mask starts fully `BLACK`, so areas with no lights are pitch black. Ambient darkness level is effectively 100%. The ship flashlight is the only guaranteed illumination.

Ship upgrade "Better Optics": flashlight radius +100px (just pass larger radius to `MoveLight`).
