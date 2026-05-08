#pragma once
#include <utils/vectors.h>
#include <flecs.h>

void CreateTimeWarpEntity(vf2d pos, float radius, float duration, float slowFactor, flecs::entity_t ownerId);
