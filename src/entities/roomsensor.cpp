#include "player.h"

#include "roomsensor.h"

void CreateRoomSensorEntity(vf2d pos, int roomId) {

	const auto& ecs = ECS::getWorld();

	flecs::entity entity;

	entity = ecs.entity()
        .set<RoomSensorEntity>({ roomId })
        .emplace<Collision>(CATEGORY_FIREBALL, MASK_FIREBALL)
        .emplace<Sprite>(32.f, 64.f, AssetHandler::GetTexture("assets/tilesets/dungeongate.png"), true, true, 32, 32, 16.f, 48.f, direction::EAST);

    vf2d outerTriggerSize = { 16, 16 };
    vf2d innerTriggerSize = { 36, 36 };

	auto userData = std::make_unique<UserData>();
	userData->entity_id = entity.id();

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_staticBody;
    bodyDef.position = {pos.x, pos.y};
    bodyDef.userData = userData.get();
	b2BodyId bodyId = World::createBody(&bodyDef);

    float roomOffset = 9.f;

    b2ShapeDef sensorDef = b2DefaultShapeDef();
    sensorDef.isSensor = true;
    sensorDef.enableSensorEvents = true;

    b2Polygon topLeft = b2MakeOffsetBox(outerTriggerSize.x / 2, outerTriggerSize.y / 2, {roomOffset, roomOffset}, b2MakeRot(0.0f));
    b2CreatePolygonShape(bodyId, &sensorDef, &topLeft);

    b2Polygon topRight = b2MakeOffsetBox(outerTriggerSize.x / 2, outerTriggerSize.y / 2, {22 + roomOffset, roomOffset}, b2MakeRot(0.0f));
    b2CreatePolygonShape(bodyId, &sensorDef, &topRight);

    b2Polygon bottomLeft = b2MakeOffsetBox(outerTriggerSize.x / 2, outerTriggerSize.y / 2, {roomOffset, 22 + roomOffset}, b2MakeRot(0.0f));
    b2CreatePolygonShape(bodyId, &sensorDef, &bottomLeft);

    b2Polygon bottomRight = b2MakeOffsetBox(outerTriggerSize.x / 2, outerTriggerSize.y / 2, {22 + roomOffset, 22 + roomOffset}, b2MakeRot(0.0f));
    b2CreatePolygonShape(bodyId, &sensorDef, &bottomRight);

    b2Polygon center = b2MakeOffsetBox(innerTriggerSize.x / 2, innerTriggerSize.y / 2, {11 + roomOffset, 11 + roomOffset}, b2MakeRot(0.0f));
    b2CreatePolygonShape(bodyId, &sensorDef, &center);

	entity.emplace<RigidBody2D>(bodyId);
	entity.set<UserDataComponent>({ std::move(userData) });

}
