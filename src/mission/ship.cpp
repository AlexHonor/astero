#include "ship.h"

void Ship::Init(const ShipConfig& cfg, Vector2 start_pos) {
    pos       = start_pos;
    vel       = {0.f, 0.f};
    rotation  = 0.f;
    hp        = cfg.max_hp;
    max_hp    = cfg.max_hp;
    cargo     = 0;
    max_cargo = cfg.max_cargo;
    thrust    = cfg.thrust;
    max_speed = cfg.max_speed;
    alive         = true;
    damage_flash  = 0.f;
}

void Ship::Update(float dt, Camera2D& cam) {
    if (!alive) return;

    // Thrust input
    Vector2 t = {0.f, 0.f};
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    t.y -= 1.f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  t.y += 1.f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  t.x -= 1.f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) t.x += 1.f;

    if (t.x != 0.f || t.y != 0.f) {
        t = Vector2Normalize(t);
        vel.x += t.x * thrust * dt;
        vel.y += t.y * thrust * dt;
    }

    // Damping
    vel.x *= (1.f - damping * dt);
    vel.y *= (1.f - damping * dt);

    // Clamp speed
    float spd = Vector2Length(vel);
    if (spd > max_speed) {
        vel = Vector2Scale(Vector2Normalize(vel), max_speed);
    }

    pos.x += vel.x * dt;
    pos.y += vel.y * dt;

    // Clamp to world bounds
    pos.x = fmaxf(20.f, fminf(3980.f, pos.x));
    pos.y = fmaxf(20.f, fminf(3980.f, pos.y));

    // Face mouse cursor
    Vector2 mouse_world = GetScreenToWorld2D(GetMousePosition(), cam);
    Vector2 dir = Vector2Subtract(mouse_world, pos);
    if (Vector2Length(dir) > 5.f) {
        rotation = atan2f(dir.y, dir.x);
    }
}

void Ship::Draw() const {
    if (!alive) return;

    // Triangle pointing in rotation direction
    float size = 12.f;
    // Tip, left wing, right wing (local space, tip at +x)
    Vector2 pts[3] = {
        {  size,      0.f},
        { -size * 0.6f, -size * 0.5f},
        { -size * 0.6f,  size * 0.5f}
    };

    float cs = cosf(rotation), sn = sinf(rotation);
    for (auto& p : pts) {
        float rx = p.x * cs - p.y * sn;
        float ry = p.x * sn + p.y * cs;
        p = {pos.x + rx, pos.y + ry};
    }

    Color ship_col = damage_flash > 0 ? RED : WHITE;
    DrawTriangle(pts[0], pts[1], pts[2], ship_col);
    DrawTriangleLines(pts[0], pts[1], pts[2], LIGHTGRAY);

    // Engine glow when thrusting
    bool thrusting = IsKeyDown(KEY_W) || IsKeyDown(KEY_S) ||
                     IsKeyDown(KEY_A) || IsKeyDown(KEY_D) ||
                     IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN) ||
                     IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT);
    if (thrusting) {
        // Exhaust dot behind ship
        float bx = -size * 0.6f, by = 0.f;
        float rx = bx * cs - by * sn;
        float ry = bx * sn + by * cs;
        DrawCircle((int)(pos.x + rx), (int)(pos.y + ry), 4.f,
                   Color{255, 140, 20, 180});
    }
}

void Ship::TakeDamage(int dmg) {
    hp -= dmg;
    damage_flash = 0.25f;
    if (hp <= 0) {
        hp = 0;
        alive = false;
    }
}

void Ship::UpdateFlash(float dt) {
    if (damage_flash > 0) damage_flash -= dt;
}
