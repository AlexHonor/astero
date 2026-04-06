#pragma once
#include "raylib.h"

enum class TileMat {
    None, Rock, Iron, Copper, Silicon, Titanium, Gold
};

struct Tile {
    TileMat material   = TileMat::None;
    int      hp         = 0;
    int      armor      = 0;
    float    exp_resist = 0.f;
    float    integrity  = 0.5f;
    float    ricochet   = 0.1f;
    bool     revealed   = false;
};

inline Color MaterialColor(TileMat m) {
    switch (m) {
        case TileMat::Rock:     return DARKGRAY;
        case TileMat::Iron:     return Color{140, 80,  50,  255};
        case TileMat::Copper:   return Color{80,  160, 90,  255};
        case TileMat::Silicon:  return Color{170, 170, 200, 255};
        case TileMat::Titanium: return Color{70,  100, 140, 255};
        case TileMat::Gold:     return GOLD;
        default:                 return Color{0, 0, 0, 0};
    }
}

inline int MaterialBaseHP(TileMat m) {
    switch (m) {
        case TileMat::Rock:     return 20;
        case TileMat::Iron:     return 40;
        case TileMat::Copper:   return 60;
        case TileMat::Silicon:  return 50;
        case TileMat::Titanium: return 120;
        case TileMat::Gold:     return 80;
        default:                 return 0;
    }
}

inline int MaterialArmor(TileMat m) {
    switch (m) {
        case TileMat::Rock:     return 0;
        case TileMat::Iron:     return 1;
        case TileMat::Copper:   return 2;
        case TileMat::Silicon:  return 1;
        case TileMat::Titanium: return 4;
        case TileMat::Gold:     return 3;
        default:                 return 0;
    }
}

inline float MaterialIntegrity(TileMat m) {
    switch (m) {
        case TileMat::Rock:     return 0.30f;
        case TileMat::Iron:     return 0.50f;
        case TileMat::Copper:   return 0.60f;
        case TileMat::Silicon:  return 0.70f;
        case TileMat::Titanium: return 0.40f;
        case TileMat::Gold:     return 0.50f;
        default:                 return 0.f;
    }
}

inline float MaterialExpResist(TileMat m) {
    switch (m) {
        case TileMat::Rock:     return 0.10f;
        case TileMat::Iron:     return 0.20f;
        case TileMat::Copper:   return 0.30f;
        case TileMat::Silicon:  return 0.15f;
        case TileMat::Titanium: return 0.60f;
        case TileMat::Gold:     return 0.40f;
        default:                 return 0.f;
    }
}

inline float MaterialRicochet(TileMat m) {
    switch (m) {
        case TileMat::Rock:     return 0.10f;
        case TileMat::Iron:     return 0.20f;
        case TileMat::Copper:   return 0.30f;
        case TileMat::Silicon:  return 0.20f;
        case TileMat::Titanium: return 0.50f;
        case TileMat::Gold:     return 0.35f;
        default:                 return 0.f;
    }
}

inline int MaterialValue(TileMat m) {
    switch (m) {
        case TileMat::Iron:     return 5;
        case TileMat::Copper:   return 15;
        case TileMat::Silicon:  return 12;
        case TileMat::Titanium: return 40;
        case TileMat::Gold:     return 80;
        default:                 return 0;
    }
}

inline Tile MakeTile(TileMat m) {
    Tile t;
    t.material   = m;
    t.hp         = MaterialBaseHP(m);
    t.armor      = MaterialArmor(m);
    t.exp_resist = MaterialExpResist(m);
    t.integrity  = MaterialIntegrity(m);
    t.ricochet   = MaterialRicochet(m);
    return t;
}

static constexpr float TILE_SIZE = 16.f;
