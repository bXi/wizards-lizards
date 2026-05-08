#include "player.h"
#include "components/playerxp.h"



void CreatePlayerEntity(int index, vf2d pos, PlayerClassType classType) {

	const auto& ecs = ECS::getWorld();

	Texture sprite = AssetHandler::GetTexture("assets/entities/entities.png");

	flecs::entity entity;
	// Create the player entity


	entity = ecs.entity()
		.set<PlayerEntity>({  })
		.set<PlayerIndex>({ index })
		.set<PlayerInput>({  })
		.set<PlayerClass>({ classType })
		.set<PlayerXP>({})
		.set<Sprite>({ 32.f, 56.f, sprite, true, true, 32, 32, 16.f, 40.f, direction::WEST })
		.set<Render2DComp>({})
		.emplace<Collision>(CATEGORY_FIREBALL, MASK_FIREBALL);

	auto userData = std::make_unique<UserData>();
	userData->entity_id = entity.id();

	// Offset each player so they don't spawn stacked
	vf2d spawnPos = { pos.x + (index - 1) * 1.5f, pos.y };

	b2BodyDef bodyDef = b2DefaultBodyDef();
	bodyDef.type = b2_dynamicBody;
	bodyDef.position = {spawnPos.x, spawnPos.y};
	bodyDef.userData = userData.get();
	b2BodyId bodyId = World::createBody(&bodyDef);

	b2ShapeDef shapeDef = b2DefaultShapeDef();
	shapeDef.density = 1.0f;
	shapeDef.enableSensorEvents = true;
	b2Circle circle = {{0.0f, 0.0f}, 0.5f};
	b2CreateCircleShape(bodyId, &shapeDef, &circle);

	entity.emplace<RigidBody2D>(bodyId);

	entity.set<UserDataComponent>({ std::move(userData) });

	entity.get_mut<PlayerClass>()->init();
	entity.get_mut<Collision>()->init(&entity);

    entity.get_mut<PlayerInput>()->init(index);

}
