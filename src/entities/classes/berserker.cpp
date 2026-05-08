#include "berserker.h"
#include "configuration.h"
#include <algorithm>
#include <cmath>

#include "components/health.h"
#include "components/damageaccumulator.h"
#include "components/whirlwindobjectdata.h"
#include "entities/whirlwindobject.h"
#include "entities/fireball.h"

void BerserkerClass::update()
{
    const float delta = Window::GetFrameTime() / Configuration::slowMotionFactor;
    secondsSinceLastCharge -= delta;
    secondsSinceLastSlam   -= delta;
    secondsSinceLastAxe    -= delta;
    hitTimer -= delta;
}

void BerserkerClass::doDamage(flecs::entity* /*entity*/, int /*weaponId*/)
{
}

void BerserkerClass::shoot(flecs::entity entity)
{
    auto* player      = entity.get_mut<PlayerInput>();
    auto* rigidBody2d = entity.get_mut<RigidBody2D>();
    b2Vec2 pos        = b2Body_GetPosition(rigidBody2d->RigidBody);

    switch (player->selectedWeapon) {

    // ── Weapon 1: Charge ─────────────────────────────────────────────────
    case 1:
    {
        if (secondsSinceLastCharge <= 0.f) {
            auto [rangeTier, damageTier, speedTier] = player->getPowerUp(0);

            float dashSpeed = 30.f + static_cast<float>(speedTier) * 4.f;
            float cooldown  = 1.5f - static_cast<float>(speedTier) * 0.1f;
            float dashRange = 4.f + static_cast<float>(rangeTier);
            float dmg       = 12.f + static_cast<float>(damageTier) * 4.f;

            vf2d dir = player->aim.norm();
            b2Body_SetLinearVelocity(rigidBody2d->RigidBody, { dir.x * dashSpeed, dir.y * dashSpeed });

            vf2d startPos = { pos.x, pos.y };
            const auto enemyFilter = ECS::getWorld().filter<EnemyEntity, RigidBody2D, Health>();
            enemyFilter.each([&](flecs::entity enemyEnt, EnemyEntity, RigidBody2D& erb, Health& h) {
                b2Vec2 ep = b2Body_GetPosition(erb.RigidBody);
                vf2d toEnemy = { ep.x - startPos.x, ep.y - startPos.y };
                float t = toEnemy.x * dir.x + toEnemy.y * dir.y;
                if (t < 0.f || t > dashRange) return;
                float px = startPos.x + dir.x * t;
                float py = startPos.y + dir.y * t;
                float distSq = (ep.x - px) * (ep.x - px) + (ep.y - py) * (ep.y - py);
                if (distSq > 1.f) return;

                h.currentHealth -= dmg;
                if (enemyEnt.has<DamageAccumulator>())
                    enemyEnt.get_mut<DamageAccumulator>()->add(dmg, { ep.x * Configuration::tileWidth, ep.y * Configuration::tileHeight });
                b2Body_ApplyLinearImpulseToCenter(erb.RigidBody, { dir.x * 800.f, dir.y * 800.f }, true);
            });

            secondsSinceLastCharge = cooldown;
            player->shooting = false;
        }
    }
    break;

    // ── Weapon 2: Whirlwind ──────────────────────────────────────────────
    case 2:
    {
        auto [countTier, damageTier, spinTier] = player->getPowerUp(1);

        int existingCount = 0;
        const auto wwFilter = ECS::getWorld().filter<WhirlwindObjectData>();
        wwFilter.each([&](flecs::entity, WhirlwindObjectData& wwd) {
            if (wwd.ownerId == entity.id()) existingCount++;
        });

        if (existingCount < 3) {
            float maxSpin   = 5.f + static_cast<float>(spinTier) * 0.8f;
            float dmg       = 6.f + static_cast<float>(damageTier) * 3.f;
            float knockback = 1800.f + static_cast<float>(damageTier) * 200.f;

            for (int slot = existingCount; slot < 3; ++slot) {
                CreateWhirlwindObjectEntity(
                    { pos.x, pos.y }, slot,
                    maxSpin, dmg, knockback,
                    entity.id());
            }
        }
        // spinSpeed ramping and orbit update handled in game.cpp
    }
    break;

    // ── Weapon 3: Ground Slam ────────────────────────────────────────────
    case 3:
    {
        if (secondsSinceLastSlam <= 0.f) {
            auto [radiusTier, damageTier, cooldownTier] = player->getPowerUp(2);

            float radius   = 3.f + static_cast<float>(radiusTier) * 0.5f;
            float dmg      = 15.f + static_cast<float>(damageTier) * 5.f;
            float cooldown = 2.5f - static_cast<float>(cooldownTier) * 0.15f;

            vf2d center  = { pos.x, pos.y };
            const float forceMag = 5000.f;

            const auto enemyFilter = ECS::getWorld().filter<EnemyEntity, RigidBody2D, Health>();
            enemyFilter.each([&](flecs::entity enemyEnt, EnemyEntity, RigidBody2D& erb, Health& h) {
                b2Vec2 ep = b2Body_GetPosition(erb.RigidBody);
                float dx = ep.x - center.x;
                float dy = ep.y - center.y;
                float distSq = dx * dx + dy * dy;
                if (distSq > radius * radius) return;

                float dist = sqrtf(distSq);
                if (dist < 0.01f) dist = 0.01f;

                float falloff = 1.f - (dist / radius);
                vf2d dir      = { dx / dist, dy / dist };

                h.currentHealth -= dmg * falloff;
                if (enemyEnt.has<DamageAccumulator>())
                    enemyEnt.get_mut<DamageAccumulator>()->add(dmg * falloff, { ep.x * Configuration::tileWidth, ep.y * Configuration::tileHeight });
                b2Body_ApplyLinearImpulseToCenter(erb.RigidBody,
                    { dir.x * forceMag * falloff, dir.y * forceMag * falloff }, true);
            });

            secondsSinceLastSlam = cooldown;
            player->shooting = false;
        }
    }
    break;

    // ── Weapon 4: Throwing Axe ───────────────────────────────────────────
    case 4:
    {
        if (secondsSinceLastAxe <= 0.f) {
            auto [bouncesTier, damageTier, speedTier] = player->getPowerUp(3);

            vf2d dir = player->aim.norm();
            vf2d vel = dir / vf2d{ static_cast<float>(Configuration::tileWidth),
                                   static_cast<float>(Configuration::tileHeight) };
            // Speed boost encoded in vel magnitude — fireball.cpp normalizes to 20 units/s
            // so we pass the direction; damage scaling is via doDamage called by world.cpp
            CreateFireballEntity(&entity, { pos.x - 0.5f, pos.y - 0.5f }, vel);

            secondsSinceLastAxe = 0.6f - static_cast<float>(speedTier) * 0.04f;
            player->shooting = false;
        }
    }
    break;

    default: break;
    }
}
