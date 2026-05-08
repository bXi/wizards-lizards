#include "debugrigidbody2d.h"
#include "configuration.h"

#include "rigidbody2d.h"
#include "luminoveau.h"
#include "utils/vectors.h"


void DebugRigidBody2D::draw(flecs::entity entity)
{
	if (entity.has<RigidBody2D>()) {

		b2BodyId bodyId = entity.get_mut<RigidBody2D>()->RigidBody;

		int shapeCount = b2Body_GetShapeCount(bodyId);
		std::vector<b2ShapeId> shapes(shapeCount);
		b2Body_GetShapes(bodyId, shapes.data(), shapeCount);

		for (b2ShapeId shapeId : shapes) {
			if (b2Shape_GetType(shapeId) == b2_polygonShape) {
				b2Polygon poly = b2Shape_GetPolygon(shapeId);
				b2Vec2 size = {poly.vertices[2].x - poly.vertices[0].x, poly.vertices[2].y - poly.vertices[0].y};

				b2AABB aabb = b2Shape_GetAABB(shapeId);
				b2Vec2 position = {(aabb.lowerBound.x + aabb.upperBound.x) * 0.5f, (aabb.lowerBound.y + aabb.upperBound.y) * 0.5f};
				position.x -= size.x / 2.f;
				position.y -= size.y / 2.f;

				Draw::RectangleFilled(
					vi2d{(int)position.x, (int)position.y} * vi2d(Configuration::tileWidth, Configuration::tileHeight),
					vi2d{(int)size.x, (int)size.y} * vi2d(Configuration::tileWidth, Configuration::tileHeight),
					color);
			}
		}
	}
}
