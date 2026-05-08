#pragma once
#include <cstdlib>
#include <utils/vectors.h>

struct DamageAccumulator {
    float accumulated = 0.f;
    float timer = 0.f;
    bool active = false;
    vf2d spawnPos = {0.f, 0.f};
    float offsetX = 0.f;

    static constexpr float WINDOW = 0.4f;

    void add(float amount, vf2d pos) {
        accumulated += amount;
        if (!active) {
            active = true;
            timer = WINDOW;
            spawnPos = pos;
            offsetX = (float)(rand() % 31) - 15.f;
        }
    }

    // Returns true when the window expires and a number should be spawned
    bool update(float dt) {
        if (!active) return false;
        timer -= dt;
        if (timer <= 0.f) {
            active = false;
            return true;
        }
        return false;
    }

    float consume() {
        float val = accumulated;
        accumulated = 0.f;
        return val;
    }
};
