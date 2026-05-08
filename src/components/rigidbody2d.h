#pragma once

#include <memory>
#include <box2d/box2d.h>


struct UserData {
	flecs::entity_t entity_id;
};

struct UserDataComponent {
	std::unique_ptr<UserData> data;
};



struct RigidBody2D
{
	b2BodyId RigidBody = b2_nullBodyId;
	float radius;
};
