#include "world.h"

#include "ecs.h"
#include "components/health.h"
#include "components/damageaccumulator.h"
#include "configuration.h"

#include "components/owner.h"
#include "components/rigidbody2d.h"
#include "components/playerclass.h"
#include "components/fragmentdata.h"
#include "components/chainlightningdata.h"
#include "components/whirlwindobjectdata.h"
#include "entities/roomsensor.h"
#include "entities/gate.h"
#include "entities/puzzleblock.h"
#include "entities/mirrorblock.h"
#include "components/pushable.h"
#include "entities/pressureplate.h"
#include <algorithm>
#include <cstdlib>

PhysicsWall::PhysicsWall(const vf2d& pos, const vf2d& size)
{
    type = ObjectType::WALL;

    ShapeColor = { 255, 0, 0, 72 };
    Size = size;
    HalfSize.x = size.x / 2;
    HalfSize.y = size.y / 2;

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_staticBody;
    bodyDef.position = {pos.x, pos.y};
    bodyDef.userData = nullptr;
    RigidBody = World::createBody(&bodyDef);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    b2Polygon box = b2MakeBox(Size.x / 2, Size.y / 2);
    b2CreatePolygonShape(RigidBody, &shapeDef, &box);
}

void PhysicsWall::draw()
{
    return;

    auto origin = (vf2d)HalfSize * vi2d(Configuration::tileWidth, Configuration::tileHeight);
    b2Vec2 p = b2Body_GetPosition(RigidBody);
    rectf rec = {p.x * Configuration::tileWidth, p.y * Configuration::tileHeight, Size.x * Configuration::tileWidth, Size.y * Configuration::tileHeight };

    vf2d drawPos = vf2d{rec.x, rec.y} + origin;
    Draw::Rectangle(drawPos, {rec.width, rec.height}, ShapeColor);
}

void World::_init()
{
    lm = LevelManager();

    dungeon = lm.getBossRoom();

    rooms = dungeon.getRooms();

    auto* leftPhysicsWall   = new PhysicsWall({ 1.0f,                             (float)dungeon.getHeight() / 2.0f }, { 2.0f                     , (float)dungeon.getHeight() });
    auto* topPhysicsWall    = new PhysicsWall({ (float)dungeon.getWidth() / 2.0f, 1.0f                              }, { (float)dungeon.getWidth(), 2.0f                       });
    auto* rightPhysicsWall  = new PhysicsWall({ (float)dungeon.getWidth() - 1.0f, (float)dungeon.getHeight() / 2.0f }, { 2.0f                     , (float)dungeon.getHeight() });
    auto* bottomPhysicsWall = new PhysicsWall({ (float)dungeon.getWidth() / 2.0f, (float)dungeon.getHeight() - 1.0f }, { (float)dungeon.getWidth(), 2.0f                       });

    World::addObject(leftPhysicsWall);
    World::addObject(topPhysicsWall);
    World::addObject(rightPhysicsWall);
    World::addObject(bottomPhysicsWall);
}

void World::_generateNewMap()
{
    lm = LevelManager();

    dungeon = lm.getDungeon();

    rooms = dungeon.getRooms();

    auto walls = mergeWalls(dungeon.getTiles(), dungeon.getWidth(), dungeon.getHeight());

    for (auto& wall : walls) {
        PhysicsWall* tempWall = nullptr;

        tempWall = new PhysicsWall({ wall.x - (wall.width/2.f), wall.y - (wall.height /2.f) }, {wall.width, wall.height});

        World::addObject(tempWall);
    }
}

static void addJitteredSegment(const vf2d& from, const vf2d& to, std::vector<vf2d>& out) {
    vf2d diff = { to.x - from.x, to.y - from.y };
    float len = sqrtf(diff.x * diff.x + diff.y * diff.y);
    if (len < 0.001f) { out.push_back(to); return; }
    vf2d perp = { -diff.y / len, diff.x / len };
    float j1 = ((rand() % 200) / 100.f - 1.f) * len * 0.25f;
    float j2 = ((rand() % 200) / 100.f - 1.f) * len * 0.25f;
    out.push_back({ from.x + diff.x * 0.33f + perp.x * j1, from.y + diff.y * 0.33f + perp.y * j1 });
    out.push_back({ from.x + diff.x * 0.66f + perp.x * j2, from.y + diff.y * 0.66f + perp.y * j2 });
    out.push_back(to);
}

static void triggerChainLightning(const vf2d& startPixelPos, flecs::entity firstEnemy,
                                   int maxChains, float chainRangeTiles, float damage) {
    const float tileW = static_cast<float>(Configuration::tileWidth);
    const float tileH = static_cast<float>(Configuration::tileHeight);

    ChainLightningData lightning;
    lightning.points.push_back(startPixelPos);

    std::vector<flecs::entity> hitEnemies;
    flecs::entity current = firstEnemy;

    for (int chain = 0; chain < maxChains && current.is_alive(); ++chain) {
        hitEnemies.push_back(current);

        b2Vec2 curB2 = b2Body_GetPosition(current.get<RigidBody2D>()->RigidBody);
        vf2d curPixel = { curB2.x * tileW, curB2.y * tileH };

        addJitteredSegment(lightning.points.back(), curPixel, lightning.points);

        current.get_mut<Health>()->currentHealth -= damage;
        if (current.has<DamageAccumulator>()) {
            current.get_mut<DamageAccumulator>()->add(damage, curPixel);
        }

        // Find nearest unhit enemy within range
        flecs::entity next = {};
        float bestDistSq = chainRangeTiles * chainRangeTiles;
        const auto enemyFilter = ECS::getWorld().filter<EnemyEntity, RigidBody2D, Health>();
        enemyFilter.each([&](flecs::entity e, EnemyEntity, RigidBody2D& rb, Health& h) {
            if (h.currentHealth <= 0.f) return;
            if (std::find(hitEnemies.begin(), hitEnemies.end(), e) != hitEnemies.end()) return;
            b2Vec2 epos = b2Body_GetPosition(rb.RigidBody);
            float dx = epos.x - curB2.x, dy = epos.y - curB2.y;
            float distSq = dx * dx + dy * dy;
            if (distSq < bestDistSq) { bestDistSq = distSq; next = e; }
        });

        if (!next.is_alive()) break;
        current = next;
    }

    if (lightning.points.size() > 1) {
        ECS::getWorld().entity().set<ChainLightningData>(lightning);
    }
}

void World::_processContactEvents()
{
    // Regular (non-sensor) contacts
    b2ContactEvents contactEvents = b2World_GetContactEvents(worldId);
    for (int i = 0; i < contactEvents.beginCount; ++i)
    {
        const b2ContactBeginTouchEvent& event = contactEvents.beginEvents[i];

        b2BodyId bodyA = b2Shape_GetBody(event.shapeIdA);
        b2BodyId bodyB = b2Shape_GetBody(event.shapeIdB);

        UserData* userDataA = reinterpret_cast<UserData*>(b2Body_GetUserData(bodyA));
        UserData* userDataB = reinterpret_cast<UserData*>(b2Body_GetUserData(bodyB));

        if (userDataA && !userDataB) {
            flecs::entity entityA = ECS::getWorld().entity(userDataA->entity_id);
            if (entityA.has<FireballEntity>() && b2Body_GetType(bodyB) == b2_staticBody) {
                entityA.set<DeleteMe>({});
            }
        }
        else if (!userDataA && userDataB) {
            flecs::entity entityB = ECS::getWorld().entity(userDataB->entity_id);
            if (entityB.has<FireballEntity>() && b2Body_GetType(bodyA) == b2_staticBody) {
                entityB.set<DeleteMe>({});
            }
        }
        else if (userDataA && userDataB) {
            flecs::entity entityA = ECS::getWorld().entity(userDataA->entity_id);
            flecs::entity entityB = ECS::getWorld().entity(userDataB->entity_id);

            if (entityA.has<EnemyEntity>() && entityB.has<EnemyEntity>()) continue;

            flecs::entity* player = nullptr;
            flecs::entity* fireball = nullptr;
            flecs::entity* enemy = nullptr;
            flecs::entity* other = nullptr;

            bool doFireball = false;
            bool doPlayerHit = false;
            bool doDeleteBulletsOnHit = false;

            if (entityA.has<EnemyEntity>() && entityB.has<FireballEntity>()) {
                enemy = &entityA; fireball = &entityB; doFireball = true;
            } else if (entityA.has<FireballEntity>() && entityB.has<EnemyEntity>()) {
                fireball = &entityA; enemy = &entityB; doFireball = true;
            } else if (entityA.has<PlayerEntity>() && entityB.has<EnemyEntity>()) {
                player = &entityA; enemy = &entityB; doPlayerHit = true;
            } else if (entityA.has<EnemyEntity>() && entityB.has<PlayerEntity>()) {
                enemy = &entityA; player = &entityB; doPlayerHit = true;
            } else if (entityA.has<FireballEntity>()) {
                fireball = &entityA; other = &entityB; doDeleteBulletsOnHit = true;
            } else if (entityB.has<FireballEntity>()) {
                fireball = &entityB; other = &entityA; doDeleteBulletsOnHit = true;
            }

            // Fireball hits puzzle/mirror block — treat like a wall
            if ((entityA.has<FireballEntity>() && (entityB.has<PuzzleBlock>() || entityB.has<MirrorBlock>())) ||
                (entityB.has<FireballEntity>() && (entityA.has<PuzzleBlock>() || entityA.has<MirrorBlock>()))) {
                flecs::entity* fb = entityA.has<FireballEntity>() ? &entityA : &entityB;
                fb->set<DeleteMe>({});
            } else

            // Player pushes any pushable block
            if ((entityA.has<PlayerEntity>() && entityB.has<Pushable>()) ||
                (entityA.has<Pushable>() && entityB.has<PlayerEntity>())) {
                flecs::entity* block  = entityA.has<Pushable>()     ? &entityA : &entityB;
                flecs::entity* player = entityA.has<PlayerEntity>() ? &entityA : &entityB;
                bool blocked = block->has<PuzzleBlock>() &&
                               (block->get<PuzzleBlock>()->fadingOut ||
                                block->get<PuzzleBlock>()->spawningIn);
                if (!blocked) {
                    auto* p = block->get_mut<Pushable>();
                    if (p->isMoving) {
                        p->pendingPushPlayerId = player->id();
                    } else {
                        p->pushChargePlayerId = player->id();
                        p->pushChargeTimer    = 0.f;
                    }
                }
            } else

            if (doFireball && enemy) {
                flecs::entity owner = ECS::getWorld().entity(fireball->get<Owner>()->owner_id);
                owner.get_mut<PlayerClass>()->doDamage(enemy, 1);
                fireball->set<DeleteMe>({});
                if (enemy->has<DamageAccumulator>()) {
                    b2Vec2 epos = b2Body_GetPosition(enemy->get<RigidBody2D>()->RigidBody);
                    enemy->get_mut<DamageAccumulator>()->add(10.f, {epos.x * Configuration::tileWidth, epos.y * Configuration::tileHeight});
                }
                enemy->get_mut<Health>()->currentHealth -= 10.f;
            }

            if (doPlayerHit && enemy) {
                player->get_mut<PlayerClass>()->playerHit(player);
            }

            if (doDeleteBulletsOnHit && other && other->has<DeleteBulletsOnHit>() && !other->get<DeleteBulletsOnHit>()->disable) {
                fireball->set<DeleteMe>({});
            }
        }
    }

    // Regular contact END events — clear push-charge when player steps off a block
    for (int i = 0; i < contactEvents.endCount; ++i)
    {
        const b2ContactEndTouchEvent& event = contactEvents.endEvents[i];

        if (!b2Shape_IsValid(event.shapeIdA) || !b2Shape_IsValid(event.shapeIdB)) continue;

        b2BodyId bodyA = b2Shape_GetBody(event.shapeIdA);
        b2BodyId bodyB = b2Shape_GetBody(event.shapeIdB);

        UserData* userDataA = reinterpret_cast<UserData*>(b2Body_GetUserData(bodyA));
        UserData* userDataB = reinterpret_cast<UserData*>(b2Body_GetUserData(bodyB));

        if (!userDataA || !userDataB) continue;

        flecs::entity entityA = ECS::getWorld().entity(userDataA->entity_id);
        flecs::entity entityB = ECS::getWorld().entity(userDataB->entity_id);

        flecs::entity* block  = nullptr;
        flecs::entity* player = nullptr;
        if (entityA.has<Pushable>() && entityB.has<PlayerEntity>()) { block = &entityA; player = &entityB; }
        else if (entityB.has<Pushable>() && entityA.has<PlayerEntity>()) { block = &entityB; player = &entityA; }

        if (block && player) {
            auto* p = block->get_mut<Pushable>();
            if (p->pushChargePlayerId == player->id() || p->pendingPushPlayerId == player->id()) {
                p->pushChargePlayerId  = 0;
                p->pushChargeTimer     = 0.f;
                p->pendingPushPlayerId = 0;
            }
        }
    }

    // Sensor contacts
    b2SensorEvents sensorEvents = b2World_GetSensorEvents(worldId);
    for (int i = 0; i < sensorEvents.beginCount; ++i)
    {
        const b2SensorBeginTouchEvent& event = sensorEvents.beginEvents[i];

        b2BodyId sensorBody = b2Shape_GetBody(event.sensorShapeId);
        b2BodyId visitorBody = b2Shape_GetBody(event.visitorShapeId);

        UserData* sensorData = reinterpret_cast<UserData*>(b2Body_GetUserData(sensorBody));
        UserData* visitorData = reinterpret_cast<UserData*>(b2Body_GetUserData(visitorBody));

        if (!sensorData || !visitorData) continue;

        flecs::entity sensorEntity = ECS::getWorld().entity(sensorData->entity_id);
        flecs::entity visitorEntity = ECS::getWorld().entity(visitorData->entity_id);

        if (sensorEntity.has<RoomSensorEntity>() && visitorEntity.has<PlayerEntity>()) {
            std::vector<flecs::entity> gateCloseEntities;
            const auto gates = ECS::getWorld().filter<GateEntity>();
            gates.each([&](flecs::entity entity, GateEntity gate) {
                if (gate.roomId == sensorEntity.get<RoomSensorEntity>()->roomId) {
                    if (!gate.closed) {
                        gateCloseEntities.push_back(entity);
                    }
                }
            });
            for (auto entity : gateCloseEntities) entity.get_mut<GateEntity>()->close(entity);
        }
        else if (sensorEntity.has<FragmentData>() && visitorEntity.has<EnemyEntity>()) {
            auto* fd = sensorEntity.get_mut<FragmentData>();
            if (fd->hp > 0.f && fd->spawnProgress >= 1.f) {
                b2Vec2 fragPos = b2Body_GetPosition(sensorEntity.get<RigidBody2D>()->RigidBody);
                vf2d fragPixel = { fragPos.x * static_cast<float>(Configuration::tileWidth),
                                   fragPos.y * static_cast<float>(Configuration::tileHeight) };
                triggerChainLightning(fragPixel, visitorEntity, fd->maxChains, fd->chainRange, fd->chainDamage);
                fd->hp = 0.f;
            }
        }
        else if (visitorEntity.has<FragmentData>() && sensorEntity.has<EnemyEntity>()) {
            auto* fd = visitorEntity.get_mut<FragmentData>();
            if (fd->hp > 0.f && fd->spawnProgress >= 1.f) {
                b2Vec2 fragPos = b2Body_GetPosition(visitorEntity.get<RigidBody2D>()->RigidBody);
                vf2d fragPixel = { fragPos.x * static_cast<float>(Configuration::tileWidth),
                                   fragPos.y * static_cast<float>(Configuration::tileHeight) };
                triggerChainLightning(fragPixel, sensorEntity, fd->maxChains, fd->chainRange, fd->chainDamage);
                fd->hp = 0.f;
            }
        }
        else if (sensorEntity.has<WhirlwindObjectData>() && visitorEntity.has<EnemyEntity>()) {
            auto* wwd = sensorEntity.get_mut<WhirlwindObjectData>();
            if (wwd->spinSpeed > 0.1f) {
                float speedRatio = wwd->spinSpeed / wwd->maxSpinSpeed;
                float dmg = wwd->damage * speedRatio;
                float impulse = wwd->knockbackForce * speedRatio;

                b2Vec2 wwPos  = b2Body_GetPosition(sensorEntity.get<RigidBody2D>()->RigidBody);
                b2Vec2 enPos  = b2Body_GetPosition(visitorEntity.get<RigidBody2D>()->RigidBody);
                float dx = enPos.x - wwPos.x, dy = enPos.y - wwPos.y;
                float len = sqrtf(dx * dx + dy * dy);
                if (len < 0.01f) { dx = 1.f; dy = 0.f; len = 1.f; }

                visitorEntity.get_mut<Health>()->currentHealth -= dmg;
                if (visitorEntity.has<DamageAccumulator>())
                    visitorEntity.get_mut<DamageAccumulator>()->add(dmg,
                        { enPos.x * Configuration::tileWidth, enPos.y * Configuration::tileHeight });
                b2Body_ApplyLinearImpulseToCenter(visitorEntity.get<RigidBody2D>()->RigidBody,
                    { (dx / len) * impulse, (dy / len) * impulse }, true);
            }
        }
        else if (visitorEntity.has<WhirlwindObjectData>() && sensorEntity.has<EnemyEntity>()) {
            auto* wwd = visitorEntity.get_mut<WhirlwindObjectData>();
            if (wwd->spinSpeed > 0.1f) {
                float speedRatio = wwd->spinSpeed / wwd->maxSpinSpeed;
                float dmg = wwd->damage * speedRatio;
                float impulse = wwd->knockbackForce * speedRatio;

                b2Vec2 wwPos  = b2Body_GetPosition(visitorEntity.get<RigidBody2D>()->RigidBody);
                b2Vec2 enPos  = b2Body_GetPosition(sensorEntity.get<RigidBody2D>()->RigidBody);
                float dx = enPos.x - wwPos.x, dy = enPos.y - wwPos.y;
                float len = sqrtf(dx * dx + dy * dy);
                if (len < 0.01f) { dx = 1.f; dy = 0.f; len = 1.f; }

                sensorEntity.get_mut<Health>()->currentHealth -= dmg;
                if (sensorEntity.has<DamageAccumulator>())
                    sensorEntity.get_mut<DamageAccumulator>()->add(dmg,
                        { enPos.x * Configuration::tileWidth, enPos.y * Configuration::tileHeight });
                b2Body_ApplyLinearImpulseToCenter(sensorEntity.get<RigidBody2D>()->RigidBody,
                    { (dx / len) * impulse, (dy / len) * impulse }, true);
            }
        }
        else if (sensorEntity.has<DeleteBulletsOnHit>() && visitorEntity.has<FireballEntity>()) {
            if (!sensorEntity.get<DeleteBulletsOnHit>()->disable) {
                visitorEntity.set<DeleteMe>({});
            }
        }
        else if (visitorEntity.has<DeleteBulletsOnHit>() && sensorEntity.has<FireballEntity>()) {
            if (!visitorEntity.get<DeleteBulletsOnHit>()->disable) {
                sensorEntity.set<DeleteMe>({});
            }
        }
        else if (sensorEntity.has<PressurePlate>() && visitorEntity.has<PlayerEntity>()) {
            sensorEntity.get_mut<PressurePlate>()->isPressed = true;
        }
        else if (visitorEntity.has<PressurePlate>() && sensorEntity.has<PlayerEntity>()) {
            visitorEntity.get_mut<PressurePlate>()->isPressed = true;
        }
    }

    // Sensor end events — used to release pressure plates
    for (int i = 0; i < sensorEvents.endCount; ++i) {
        const b2SensorEndTouchEvent& event = sensorEvents.endEvents[i];

        if (!b2Shape_IsValid(event.sensorShapeId) || !b2Shape_IsValid(event.visitorShapeId)) continue;

        b2BodyId sensorBody  = b2Shape_GetBody(event.sensorShapeId);
        b2BodyId visitorBody = b2Shape_GetBody(event.visitorShapeId);

        UserData* sensorData  = reinterpret_cast<UserData*>(b2Body_GetUserData(sensorBody));
        UserData* visitorData = reinterpret_cast<UserData*>(b2Body_GetUserData(visitorBody));

        if (!sensorData || !visitorData) continue;

        flecs::entity sensorEntity  = ECS::getWorld().entity(sensorData->entity_id);
        flecs::entity visitorEntity = ECS::getWorld().entity(visitorData->entity_id);

        if (sensorEntity.has<PressurePlate>() && visitorEntity.has<PlayerEntity>()) {
            sensorEntity.get_mut<PressurePlate>()->isPressed = false;
        }
        else if (visitorEntity.has<PressurePlate>() && sensorEntity.has<PlayerEntity>()) {
            visitorEntity.get_mut<PressurePlate>()->isPressed = false;
        }
    }
}

//CREDIT: piratux on Javids Discord
std::vector<rectf> World::mergeWalls(const std::vector<Tile>& map, int width, int height) {

    std::vector<rectf> horizontalRectangles;
    std::vector<rectf> verticalRectangles;


    std::vector<rectf> mergedRectangles;


    // TODO: replace "asset_manager->map[index]" with get function
    auto create_tile_hitbox = [&](int32_t x_idx, int32_t y_idx, int32_t x_length, int32_t y_length, bool horizontal_hitbox)
    {
        rectf box = {
            static_cast<float>(x_idx),
            static_cast<float>(y_idx),
            static_cast<float>(x_length),
            static_cast<float>(y_length)
        };

        if (horizontal_hitbox)
            horizontalRectangles.push_back(box);
        else
            verticalRectangles.push_back(box);
    };

    // chain length of wall tiles next to each other counted in tiles
    int32_t chain_length;

    int32_t height_idx, width_idx;

    // horizontal iteration

    for (height_idx = 0; height_idx < height; height_idx++)
    {
        chain_length = 0;
        for (width_idx = 0; width_idx < width; width_idx++)
        {
            int32_t index = width * height_idx + width_idx;

            // 0 means empty tile
            if (map[index] == Tile::FLOOR)
            {
                if (chain_length > 0)
                {
                    create_tile_hitbox(width_idx, height_idx+1, chain_length, 1, true);
                    chain_length = 0;
                }
            }
            else
            {
                // if chain hasn't started
                if (chain_length == 0)
                {
                    // if hitbox length would be > 1
                    if (width_idx + 1 != width)
                    {
                        int32_t right_index = width * height_idx + width_idx + 1;
                        if (map[right_index] != Tile::FLOOR)
                        {
                            ++chain_length;
                            continue;
                        }
                    }

                    // if there is a wall tile on top or on bottom from this tile, don't
                    // create hitbox, because vertical iteration will create the hitbox
                    bool top_or_bottom_tile_exists = false;

                    if (height_idx != 0)
                    {
                        int32_t top_index = width * (height_idx - 1) + width_idx;
                        if (map[top_index] != Tile::FLOOR)
                            top_or_bottom_tile_exists = true;
                    }
                    if (height_idx + 1 != height)
                    {
                        int32_t bottom_index = width * (height_idx + 1) + width_idx;
                        if (map[bottom_index] != Tile::FLOOR)
                            top_or_bottom_tile_exists = true;
                    }

                    if (!top_or_bottom_tile_exists)
                        ++chain_length;
                }
                else
                {
                    ++chain_length;
                }
            }
        }

        if (chain_length > 0)
        {
            create_tile_hitbox(width_idx, height_idx + 1, chain_length, 1, true);
            chain_length = 0;
        }
    }

    // vertical iteration

    for (width_idx = 0; width_idx < width; width_idx++)
    {
        chain_length = 0;
        for (height_idx = 0; height_idx < height; height_idx++)
        {
            int32_t index = width * height_idx + width_idx;

            // 0 means empty tile
            if (map[index] == Tile::FLOOR)
            {
                if (chain_length > 0)
                {
                    create_tile_hitbox(width_idx + 1, height_idx, 1, chain_length, false);
                    chain_length = 0;
                }
            }
            else
            {
                // if chain hasn't started
                if (chain_length == 0)
                {
                    // if hitbox length would be > 1
                    if (height_idx + 1 != height)
                    {
                        int32_t bottom_index = width * (height_idx + 1) + width_idx;
                        if (map[bottom_index] != Tile::FLOOR)
                            ++chain_length;
                    }
                }
                else
                {
                    ++chain_length;
                }
            }
        }

        if (chain_length > 0)
        {
            create_tile_hitbox(width_idx + 1, height_idx, 1, chain_length, false);
            chain_length = 0;
        }
    }

    for (const auto& horizontalRect : horizontalRectangles) {
        if (horizontalRect.width > 2) mergedRectangles.push_back(horizontalRect);
    }

    for (const auto& verticalRect : verticalRectangles) {
        if (verticalRect.height > 2) mergedRectangles.push_back(verticalRect);
    }


    return mergedRectangles;

}


void World::_removeObject(PhysicsObject* object)
{
    auto it = std::find(objects.begin(), objects.end(), object);
    if (it != objects.end()) {
        if (b2Body_IsValid(object->RigidBody)) {
            b2DestroyBody(object->RigidBody);
        }
        objects.erase(it);
        delete object;
    }
}

void World::_clear() {

}
