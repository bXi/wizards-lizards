#include "fragment.h"
#include "ecs.h"
#include "world/world.h"
#include "components/fragmentdata.h"
#include "components/rigidbody2d.h"
#include "components/type.h"

void CreateFragmentEntity(vf2d pos, float maxHp, float maxLifetime,
                          int maxChains, float chainRange, float chainDamage,
                          flecs::entity_t ownerId) {
    const auto& ecs = ECS::getWorld();

    flecs::entity entity = ecs.entity()
        .set<FragmentData>({ ownerId, maxHp, maxHp, maxLifetime, maxLifetime,
                             0.f, maxChains, chainRange, chainDamage });

    auto userData = std::make_unique<UserData>();
    userData->entity_id = entity.id();

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_kinematicBody;
    bodyDef.position = { pos.x, pos.y };
    bodyDef.userData = userData.get();
    b2BodyId bodyId = World::createBody(&bodyDef);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.isSensor = true;
    shapeDef.enableSensorEvents = true;
    b2Circle circle = { { 0.f, 0.f }, 0.3f };
    b2CreateCircleShape(bodyId, &shapeDef, &circle);

    entity.emplace<RigidBody2D>(bodyId, 0.3f);
    entity.set<UserDataComponent>({ std::move(userData) });
}
