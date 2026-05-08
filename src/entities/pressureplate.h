#pragma once

#include "ecs.h"
#include "world/world.h"
#include "components/rigidbody2d.h"
#include "map/room.h"

struct PressurePlate {
    int  roomId    = -1;
    bool isPressed = false;
    vi2d tilePos   = {};  // top-left tile of the plate (for drawing)
};

// Creates numPlates pressure plate sensors at the room's interior corners.
// numPlates is clamped to [1, 4].
void CreatePressurePlateEntities(Room* room, int numPlates);
