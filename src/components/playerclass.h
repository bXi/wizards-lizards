#pragma once

#include "ecs.h"
#include "rigidbody2d.h"

#include "entities/classes/baseclass.h"
#include "entities/classes/wizard.h"
#include "entities/classes/berserker.h"
#include "world/world.h"


enum class PlayerClassType
{
	WIZARD,
	WRONG_WIZARD,
	PINBALL_WIZARD,
	FROST_WIZARD

};


struct PlayerClass
{

	PlayerClassType classtype;


	BaseClass* selectedClass;

	void init()
	{

		switch (classtype)
		{
		case  PlayerClassType::WIZARD:        selectedClass = new WizardClass(); break;
		case  PlayerClassType::WRONG_WIZARD:  selectedClass = new BerserkerClass(); break;
		case  PlayerClassType::PINBALL_WIZARD: break;
		case  PlayerClassType::FROST_WIZARD:  break;
		}
	}


	void doDamage(flecs::entity* entity, int weaponId)
	{
		if (selectedClass) selectedClass->doDamage(entity, weaponId);
	}

	int getSpriteIndex() const
	{
		if (selectedClass) return selectedClass->getSpriteIndex();
		return -1;
	}

	void shoot(flecs::entity entity)
	{
		if (selectedClass) selectedClass->shoot(entity);
	}

	void update()
	{
		if (selectedClass) selectedClass->update();
	}

	int getRenderFrame(flecs::entity* entity);


	float hitTimer = 0.0f;
	void playerHit(flecs::entity* entity)
	{

		World::startSlowMotion();
		hitTimer = 0.125f;

		auto rigidBody2d = entity->get<RigidBody2D>();

		float radius = 5.f;
		float forceMagnitude = 4500.f;

		b2Vec2 b2pos = b2Body_GetPosition(rigidBody2d->RigidBody);
		vf2d pos = {b2pos.x, b2pos.y};

		const auto enemyFilter = ECS::getWorld().filter<EnemyEntity>();
		enemyFilter.each([&](flecs::entity enemy, EnemyEntity renderer) {
			auto* enemyRigidBody2d = enemy.get<RigidBody2D>();

			b2Vec2 mp = b2Body_GetPosition(enemyRigidBody2d->RigidBody);
			vf2d monsterPosition = {mp.x, mp.y};

			float distance = (pos - monsterPosition).mag();
			if (distance <= radius) {
				vf2d forceDirection = (monsterPosition - pos).norm();
				vf2d force = forceDirection * forceMagnitude;

				b2Body_ApplyForceToCenter(enemyRigidBody2d->RigidBody, {force.x, force.y}, true);
			}

		});


	}

};