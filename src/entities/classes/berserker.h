#pragma once

#include "baseclass.h"

class BerserkerClass : public BaseClass {
    float secondsSinceLastCharge = 0.f;
    float secondsSinceLastSlam   = 0.f;
    float secondsSinceLastAxe    = 0.f;

public:
    int getSpriteIndex() override { return 17; }

    void doDamage(flecs::entity* entity, int weaponId) override;
    void update() override;
    void shoot(flecs::entity entity) override;
};
