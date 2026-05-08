#include "lizard.h"
#include "components/damageaccumulator.h"

void CreateLizardEntity(vf2d pos, float maxHealth) {

	const auto& ecs = ECS::getWorld();

	Texture sprite = AssetHandler::GetTexture("assets/monsters/lizard/lizard.png");

	flecs::entity entity;

	entity = ecs.entity()
		.set<Health>({ maxHealth, 32 })
		.set<EnemyEntity>({ })
		.set<AIController>({})
		.set<Sprite>({ 32.f, 64.f, sprite, false, false, 16, 16, 16.f, 48.f })
		.set<Render2DComp>({  });

	auto userData = std::make_unique<UserData>();
	userData->entity_id = entity.id();

	b2BodyDef bodyDef = b2DefaultBodyDef();
	bodyDef.type = b2_dynamicBody;
	bodyDef.position = {pos.x, pos.y};
	bodyDef.userData = userData.get();
	b2BodyId bodyId = World::createBody(&bodyDef);

	b2ShapeDef shapeDef = b2DefaultShapeDef();
	shapeDef.density = 1.0f;
	shapeDef.enableContactEvents = true;
	shapeDef.enableSensorEvents = true;
	constexpr float radius = 0.5f;
	b2Circle circle = {{0.0f, 0.0f}, radius};
	b2CreateCircleShape(bodyId, &shapeDef, &circle);

	entity.emplace<RigidBody2D>(bodyId, radius);

	entity.set<UserDataComponent>({ std::move(userData) });

	entity.set<DamageAccumulator>({});
	entity.get_mut<AIController>()->self = entity;


}
