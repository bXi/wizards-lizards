#include "pressureplate.h"
#include <luminoveau.h>

void CreatePressurePlateEntities(Room* room, int numPlates) {
    numPlates = std::max(1, std::min(numPlates, 4));

    // Four corners of the room interior, inset 2 tiles from walls.
    // Order: TL, BR, TR, BL — so pairs (TL+BR, TR+BL) are diagonally opposite,
    // forcing players to split across the room.
    const vi2d corners[4] = {
        { room->pos.x + 2,                    room->pos.y + 2                    }, // TL
        { room->pos.x + room->size.x - 3,     room->pos.y + room->size.y - 3     }, // BR
        { room->pos.x + room->size.x - 3,     room->pos.y + 2                    }, // TR
        { room->pos.x + 2,                    room->pos.y + room->size.y - 3     }, // BL
    };

    auto& ecs = ECS::getWorld();

    for (int i = 0; i < numPlates; ++i) {
        vi2d tile = corners[i];

        auto entity = ecs.entity();

        auto userData = std::make_unique<UserData>();
        userData->entity_id = entity.id();

        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type     = b2_staticBody;
        bodyDef.position = { tile.x + 0.5f, tile.y + 0.5f };
        bodyDef.userData = userData.get();
        b2BodyId bodyId  = World::createBody(&bodyDef);

        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.isSensor           = true;
        shapeDef.enableSensorEvents = true;
        b2Polygon box = b2MakeBox(0.6f, 0.6f);
        b2CreatePolygonShape(bodyId, &shapeDef, &box);

        PressurePlate pp;
        pp.roomId  = room->id;
        pp.tilePos = tile;

        entity.emplace<RigidBody2D>(bodyId)
              .set<PressurePlate>(pp)
              .set<UserDataComponent>({ std::move(userData) });
    }
}
