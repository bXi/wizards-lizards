#pragma once
#include <utils/vectors.h>
#include <flecs.h>

struct TimeWarpData {
    vf2d position;              // world tile units (center)
    float radius = 3.0f;        // tile units
    float timer = 0.f;
    float maxTimer = 4.0f;
    float slowFactor = 0.15f;
    flecs::entity_t ownerId = 0;
};
