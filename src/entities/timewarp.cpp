#include "timewarp.h"
#include "ecs.h"
#include "components/timewarpdata.h"

void CreateTimeWarpEntity(vf2d pos, float radius, float duration, float slowFactor, flecs::entity_t ownerId) {
    ECS::getWorld().entity()
        .set<TimeWarpData>({ pos, radius, duration, duration, slowFactor, ownerId });
}
