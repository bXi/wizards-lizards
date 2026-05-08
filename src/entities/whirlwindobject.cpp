#include "whirlwindobject.h"
#include "ecs.h"
#include "world/world.h"
#include "components/whirlwindobjectdata.h"
#include "components/rigidbody2d.h"
#include "components/type.h"
#include <cmath>

void CreateWhirlwindObjectEntity(vf2d pos, int slotIndex,
                                  float maxSpinSpeed, float damage, float knockbackForce,
                                  flecs::entity_t ownerId) {
    const auto& ecs = ECS::getWorld();

    float baseAngle = static_cast<float>(slotIndex) * (2.f * PI / 3.f);

    flecs::entity entity = ecs.entity()
        .set<WhirlwindObjectData>({
            ownerId,
            baseAngle,
            1.5f,          // orbitRadius
            slotIndex,
            0.f,           // spinSpeed starts at 0
            maxSpinSpeed,
            damage,
            knockbackForce
        });

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
    b2Circle circle = { { 0.f, 0.f }, 0.35f };
    b2CreateCircleShape(bodyId, &shapeDef, &circle);

    entity.emplace<RigidBody2D>(bodyId, 0.35f);
    entity.set<UserDataComponent>({ std::move(userData) });
}
