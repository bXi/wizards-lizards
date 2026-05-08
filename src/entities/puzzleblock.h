#pragma once

#include "ecs.h"
#include "world/world.h"
#include "components/rigidbody2d.h"
#include "components/pushable.h"
#include "map/room.h"

inline constexpr Color PUZZLE_BLOCK_COLORS[4] = {
    { 220, 60,  60,  255 }, // Red
    { 60,  110, 220, 255 }, // Blue
    { 60,  200, 80,  255 }, // Green
    { 220, 190, 40,  255 }, // Yellow
};
inline constexpr int PUZZLE_BLOCK_COLOR_COUNT = 3;
inline constexpr int PUZZLE_GRID_STEP = 1; // one tile per push

struct PuzzleBlock {
    int  roomId        = -1;
    int  colorIdx      = 0;
    bool fadingOut     = false;
    float fadeTimer    = 0.f;
    vi2d  initialTilePos = {}; // original spawn tile — used for puzzle reset
    bool  spawningIn     = false;
    float spawnFadeTimer = 0.f;

    static constexpr float FADE_DURATION    = 0.40f;
    static constexpr float FADE_IN_DURATION = 0.50f;
};

void CreatePuzzleBlockEntity(vf2d tilePos, int colorIdx, int roomId, bool spawnFadeIn = false);
// outInitial is filled with (tilePos, colorIdx) for every block spawned, used for reset.
void SpawnPuzzleForRoom(Room* room, std::vector<std::pair<vf2d, int>>* outInitial = nullptr);
