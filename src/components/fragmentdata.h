#pragma once
#include <flecs.h>

struct FragmentData {
    flecs::entity_t ownerId = 0;
    float hp = 2.f;
    float maxHp = 2.f;
    float lifetime = 8.f;
    float maxLifetime = 8.f;
    float spawnProgress = 0.f;
    int   maxChains = 1;
    float chainRange = 3.f;    // tile units
    float chainDamage = 5.f;

    static constexpr float SPAWN_DURATION = 0.35f;
};
