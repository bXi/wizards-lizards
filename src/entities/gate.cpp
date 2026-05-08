#include "player.h"

#include "gate.h"

void CreateGateEntity(vf2d pos, int roomId) {

	const auto& ecs = ECS::getWorld();

	Texture sprite = AssetHandler::GetTexture("assets/tilesets/dungeongate.png");

	flecs::entity entity;

	entity = ecs.entity()
        .set<DeleteBulletsOnHit>({})
        .set<GateEntity>({ roomId })
        .set<RenderFrame>({ 16 })
		.set<Render2DComp>({})
        .emplace<Collision>(CATEGORY_FIREBALL, (uint16_t)0x0000)
        .emplace<Sprite>(32.f, 64.f, sprite, true, true, 32, 32, 16.f, 48.f, direction::EAST);

	auto userData = std::make_unique<UserData>();
	userData->entity_id = entity.id();

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_staticBody;
    bodyDef.position = {pos.x, pos.y};
    bodyDef.userData = userData.get();
	b2BodyId bodyId = World::createBody(&bodyDef);

	entity.emplace<RigidBody2D>(bodyId);

	entity.set<UserDataComponent>({ std::move(userData) });

    entity.get_mut<GateEntity>()->open(entity);

}
