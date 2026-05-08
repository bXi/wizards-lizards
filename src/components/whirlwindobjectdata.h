#pragma once
#include "flecs.h"

struct WhirlwindObjectData {
    flecs::entity_t ownerId = 0;
    float orbitAngle = 0.f;       // current absolute angle in radians
    float orbitRadius = 1.5f;     // tile units
    int slotIndex = 0;            // 0, 1, or 2
    float spinSpeed = 0.f;        // current spin speed in radians/sec
    float maxSpinSpeed = 5.f;     // max spin speed (scales with upgrades)
    float damage = 5.f;           // damage per hit (scales with upgrades)
    float knockbackForce = 1800.f; // impulse at full spin
};
