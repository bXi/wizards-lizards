#pragma once

#include "box2d/box2d.h"
#include "utils/vectors.h"
#include "utils/colors.h"

#include "luminoveau.h"
#include "entities/physicsobject.h"

extern b2WorldId testWorldId;

class BoxObject : public PhysicsObject
{
public:

    vf2d Size = { 0,0 };
    vf2d HalfSize = { 0,0 };

    BoxObject(vf2d pos, vf2d size, Color c, float angle = 0, bool isDynamic = true);
    void draw() override;
};

class BallObject : public PhysicsObject
{
public:
    float Radius = 0;
    b2ShapeId circleShapeId = b2_nullShapeId;

    BallObject(vf2d pos, float radius, Color c);
    void draw() override;
};
