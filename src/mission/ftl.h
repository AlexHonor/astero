#pragma once

class Ship;

class FTLSystem {
public:
    bool  charging        = false;
    float charge_progress = 0.f;   // 0–1
    float charge_time     = 8.f;
    bool  on_cooldown     = false;
    float cooldown_left   = 0.f;
    static constexpr float COOLDOWN = 30.f;

    void Init(float charge_secs) { charge_time = charge_secs; }

    void TryActivate() {
        if (!on_cooldown && !charging)
            charging = true;
    }

    // Call when ship takes damage during charge
    void Interrupt() {
        if (charging) {
            charging        = false;
            charge_progress = 0.f;
            on_cooldown     = true;
            cooldown_left   = COOLDOWN;
        }
    }

    void Update(float dt) {
        if (on_cooldown) {
            cooldown_left -= dt;
            if (cooldown_left <= 0.f) on_cooldown = false;
            return;
        }
        if (!charging) return;
        charge_progress += dt / charge_time;
        if (charge_progress > 1.f) charge_progress = 1.f;
    }

    bool IsComplete() const { return charge_progress >= 1.f; }
};
