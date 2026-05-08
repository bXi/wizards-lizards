#pragma once

#include "utils/vectors.h"

// Shared component for any entity that can be pushed one tile at a time by a player.
// Attach this alongside the block-type component (PuzzleBlock, MirrorBlock, etc.).
struct Pushable {
    // Slide animation state
    bool  isMoving  = false;
    bool  settled   = true;
    vf2d  startPos  = {};
    vf2d  targetPos = {};
    float moveTimer = 0.f;

    // Push-input state
    uint64_t pendingPushPlayerId = 0;  // player in contact while block was already sliding
    uint64_t pushChargePlayerId  = 0;  // player currently holding against idle block
    float    pushChargeTimer     = 0.f;
    float    pushCooldown        = 0.f; // post-push lock-out: ignores all input while > 0

    static constexpr float MOVE_DURATION        = 0.22f;
    static constexpr float PUSH_CHARGE_DURATION = 0.28f;
    static constexpr float PUSH_COOLDOWN        = 0.08f;
};
