#pragma once
#include <vector>
#include <utils/vectors.h>

struct ChainLightningData {
    std::vector<vf2d> points; // world-pixel positions, pre-jittered, drawn as consecutive line segments
    float timer = 0.f;
    static constexpr float MAX_TIMER = 0.45f;
};
