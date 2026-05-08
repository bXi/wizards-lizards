#pragma once

#include "luminoveau.h"

// One distinct color per player slot — used for the shadow ring indicator
inline constexpr Color PLAYER_COLORS[4] = {
    { 80, 160, 255, 200 }, // P1 blue
    { 255,  80,  80, 200 }, // P2 red
    { 80, 220,  80, 200 }, // P3 green
    { 255, 200,  50, 200 }, // P4 yellow
};

struct PlayerIndex
{
	int index;
};