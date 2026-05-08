#include "box2dobjects.h"

#ifndef M_PI
#define M_PI 3.1415629
#endif

BoxObject::BoxObject(vf2d pos, vf2d size, Color c, float angle, bool isDynamic)
{
    ShapeColor = c;
    Size = size;
    HalfSize.x = size.x / 2;
    HalfSize.y = size.y / 2;

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = isDynamic ? b2_dynamicBody : b2_staticBody;
    bodyDef.position = {pos.x, pos.y};
    bodyDef.rotation = b2MakeRot(angle * (float)(M_PI / 180.0));
    bodyDef.userData = nullptr;
    RigidBody = b2CreateBody(testWorldId, &bodyDef);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    b2Polygon box = b2MakeBox(Size.x / 2, Size.y / 2);
    b2CreatePolygonShape(RigidBody, &shapeDef, &box);
}

void BoxObject::draw()
{
    //TODO: implement rotated rectangle
}

BallObject::BallObject(vf2d pos, float radius, Color c)
{
    Radius = radius;
    ShapeColor = c;

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = {pos.x, pos.y};
    bodyDef.userData = this;
    RigidBody = b2CreateBody(testWorldId, &bodyDef);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    b2Circle circle = {{0.0f, 0.0f}, radius};
    circleShapeId = b2CreateCircleShape(RigidBody, &shapeDef, &circle);
}

void BallObject::draw()
{
    //DrawCircleV(vf2d{ ... }, Radius, ShapeColor);
}
