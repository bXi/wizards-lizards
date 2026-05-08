#pragma once
#include <utils/vectors.h>
#include <flecs.h>

void CreateFragmentEntity(vf2d pos, float maxHp, float maxLifetime,
                          int maxChains, float chainRange, float chainDamage,
                          flecs::entity_t ownerId);
