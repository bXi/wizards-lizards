#pragma once
#include <cstdint>
#include <vector>

#include "rigidbody2d.h"

const uint16_t CATEGORY_FIREBALL = 0x0002;
const uint16_t MASK_FIREBALL = 0x0001;

struct Collision
{

	uint16_t category;
	uint16_t mask;


	Collision(uint16_t _category, uint16_t _mask) : category(_category), mask(_mask)
	{
		
	}

	void init(flecs::entity* entity)
	{
		b2BodyId rigidBody = entity->get_mut<RigidBody2D>()->RigidBody;

		b2Filter filter = b2DefaultFilter();
		filter.categoryBits = category;
		filter.maskBits = mask;

		int shapeCount = b2Body_GetShapeCount(rigidBody);
		std::vector<b2ShapeId> shapes(shapeCount);
		b2Body_GetShapes(rigidBody, shapes.data(), shapeCount);
		for (b2ShapeId shapeId : shapes) {
			b2Shape_SetFilter(shapeId, filter);
		}
	}
	
};
