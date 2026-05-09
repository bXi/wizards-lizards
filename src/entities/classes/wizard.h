#pragma once

#include <vector>
#include "baseclass.h"
#include "luminoveau.h"


class WizardClass : public BaseClass
{

    struct lineSegment
    {
        float sx, sy;
        float ex, ey;
        float radius;
    };

    std::vector<lineSegment> vecLines;

    struct WallHit {
        vf2d pos;
        vf2d outDir;
    };

    static constexpr int LASER_PARTICLE_COUNT = 12;
    ParticleSystemHandle m_laserHitParticles[LASER_PARTICLE_COUNT];
    bool                 m_laserParticlesInited = false;
    std::vector<WallHit> m_wallHits;
    ParticleSystemConfig m_laserHitCfg;

    float secondsSinceLastShot = 0.0f;
    float laserLastDamageTick = 0.0f;
    float secondsSinceLastWarp = 0.0f;
    float secondsSinceLastFragment = 0.0f;

public:
    int getSpriteIndex() override
    {
        return 16;
    }

    void doDamage(flecs::entity* entity, int weaponId) override;
    void update() override;
    void shoot(flecs::entity entity) override;

};
