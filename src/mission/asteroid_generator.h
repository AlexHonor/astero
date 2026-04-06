#pragma once
#include "asteroid.h"
#include <vector>
#include <cstdint>

struct FieldConfig {
    int   num_asteroids   = 12;
    float field_radius    = 1600.f;
    float min_dist        = 220.f;
    int   min_size        = 18;
    int   max_size        = 52;
    float deep_mat_bias   = 0.1f;   // extra chance of rich materials
};

class AsteroidGenerator {
public:
    // Fills 'field' with freshly generated asteroids
    static void Generate(std::vector<Asteroid>& field, FieldConfig cfg, uint32_t seed);

private:
    static void BuildAsteroid(Asteroid& ast, int w, int h, uint32_t seed, float deep_bias);
    static float ValueNoise(float x, uint32_t seed);
    static float PolarRadius(float theta, float base_r, uint32_t seed);
};
