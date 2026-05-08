#pragma once
#include "utils/vectors.h"
#include "flecs.h"

void CreateWhirlwindObjectEntity(vf2d pos, int slotIndex,
                                  float maxSpinSpeed, float damage, float knockbackForce,
                                  flecs::entity_t ownerId);
