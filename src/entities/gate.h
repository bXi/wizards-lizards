#pragma once

#include "ecs.h"
#include "world/world.h"

#include "components/collision.h"
#include "components/playerindex.h"
#include "components/rigidbody2d.h"
#include "components/playerclass.h"
#include "components/render2d.h"
#include "components/playerinput.h"
#include "components/deletebulletsonhit.h"
#include "components/renderframe.h"

struct GateEntity {
    int roomId = -1;
    bool closed = true;
    float openTimer = 0.f;
    b2ShapeId shapeId = b2_nullShapeId;

    void _recreateShape(b2BodyId bodyId, bool isSensor) {
        if (b2Shape_IsValid(shapeId)) {
            b2DestroyShape(shapeId, false);
        }
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.isSensor = isSensor;
        shapeDef.density = 1.0f;
        b2Polygon box = b2MakeBox(0.5f, 0.5f);
        shapeId = b2CreatePolygonShape(bodyId, &shapeDef, &box);
    }

    void close(flecs::entity entity) {
        entity.get_mut<GateEntity>()->closed = true;
        entity.get_mut<RenderFrame>()->RenderFrameNumber = 20;
        b2BodyId bodyId = entity.get_mut<RigidBody2D>()->RigidBody;
        entity.get_mut<GateEntity>()->_recreateShape(bodyId, false);
        entity.get_mut<DeleteBulletsOnHit>()->disable = false;
    }

    void open(flecs::entity entity) {
        entity.get_mut<GateEntity>()->closed = false;
        entity.get_mut<RenderFrame>()->RenderFrameNumber = 16;
        b2BodyId bodyId = entity.get_mut<RigidBody2D>()->RigidBody;
        entity.get_mut<GateEntity>()->_recreateShape(bodyId, true);
        entity.get_mut<DeleteBulletsOnHit>()->disable = true;
    }
};

void CreateGateEntity(vf2d pos, int roomId);

