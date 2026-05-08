#include "fireball.h"

void CreateFireballEntity(flecs::entity* player, vf2d pos, vf2d vel) {

	const auto& ecs = ECS::getWorld();

	Texture sprite = AssetHandler::GetTexture("assets/players/wizard/fireball.png");

	flecs::entity entity;

	entity = ecs.entity()
		.set<FireballEntity>({ })
		.set<Sprite>({ 8.f, 8.f, sprite })
		.set<Render2DComp>({  });

	auto userData = std::make_unique<UserData>();
	userData->entity_id = entity.id();

	b2BodyDef bodyDef = b2DefaultBodyDef();
	bodyDef.type = b2_dynamicBody;
	bodyDef.position = {pos.x, pos.y};
	bodyDef.gravityScale = 0.0f;
	bodyDef.userData = userData.get();
	b2BodyId bodyId = World::createBody(&bodyDef);

	b2ShapeDef shapeDef = b2DefaultShapeDef();
	shapeDef.density = 1.0f;
	shapeDef.enableContactEvents = true;
	b2Circle circle = {{0.0f, 0.0f}, 0.125f};
	b2CreateCircleShape(bodyId, &shapeDef, &circle);

	entity.emplace<RigidBody2D>(bodyId);

	entity.set<UserDataComponent>({ std::move(userData) });

	entity.emplace<Collision>(CATEGORY_FIREBALL, MASK_FIREBALL);

	entity.get_mut<Collision>()->init(&entity);

	entity.emplace<Owner>(player->id());

	vf2d velocity = vel.norm() * 20;
	b2Body_SetLinearVelocity(bodyId, {velocity.x, velocity.y});
}
