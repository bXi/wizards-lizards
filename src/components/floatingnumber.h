#pragma once
#include <string>
#include <utils/vectors.h>
#include <luminoveau.h>

struct FloatingNumberData {
    std::string text;
    Color color = WHITE;
    float timer = 0.f;
    float maxTimer = 1.0f;
    vf2d worldPixelPos = {0.f, 0.f};
    float offsetX = 0.f;
    bool isDeath = false;
};
