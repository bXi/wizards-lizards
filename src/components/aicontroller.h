#pragma once
#include "configuration.h"

struct AIController
{
	flecs::entity target;
	flecs::entity self;

	float maxSpeed = 4.0f;

	flecs::entity selectRandomTarget() {

		std::vector<flecs::entity> entities;

		const auto playerFilter = ECS::getWorld().filter<PlayerIndex>();
		playerFilter.each([&](flecs::entity entity, PlayerIndex index) {
			entities.push_back(entity);
		});

		std::random_device rd;
		std::mt19937 generator(rd());
		std::uniform_int_distribution<int> distribution(0, static_cast<int>(entities.size()) - 1);
		return entities[distribution(generator)];
	}

	AIController()
	{
		target = selectRandomTarget();
	}

	void applyForce(const vf2d& desiredVelocity)
	{
		b2BodyId bodyId = self.get<RigidBody2D>()->RigidBody;
		const b2Vec2 currentVelocity = b2Body_GetLinearVelocity(bodyId);
		const b2Vec2 steering = b2Vec2{desiredVelocity.x, desiredVelocity.y} - currentVelocity;
		const b2Vec2 force = b2Vec2{steering.x * maxSpeed, steering.y * maxSpeed};
		b2Body_ApplyForceToCenter(bodyId, force, true);
	}

	void seek()
	{
		b2Vec2 targetPos = b2Body_GetPosition(target.get<RigidBody2D>()->RigidBody);
		b2Vec2 selfPos   = b2Body_GetPosition(self.get<RigidBody2D>()->RigidBody);
		const vf2d desired = vf2d{targetPos.x - selfPos.x, targetPos.y - selfPos.y}.norm() * maxSpeed / Configuration::slowMotionFactor;
		applyForce(desired);
	}


};