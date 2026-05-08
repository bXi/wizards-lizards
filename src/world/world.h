#pragma once

#include <vector>
#include <audio/audiohandler.h>
#include "configuration.h"


#include "map/levelmanager.h"
#include "utils/lerp.h"
#include "box2d/box2d.h"
#include "entities/physicsobject.h"
#include "map/room.h"
#include "flecs.h"
#include "components/type.h"
#include "components/deletebulletsonhit.h"
#include "map/dungeon.h"

class PhysicsWall : public PhysicsObject
{
public:
    vf2d Size = { 0,0 };
    vf2d HalfSize = { 0,0 };

    PhysicsWall(const vf2d& pos, const vf2d& size);
    void draw() override;
};



class World {
public:

    static void init()
    {
        get()._init();
    }

    static void clear()
    {
        get()._clear();
    }

    static std::vector<Room*> getRooms()
    {
        return get().rooms;
    }

    static Color getBackgroundColor()
    {
        return get()._backgroundColor;
    }

    static b2BodyId createBody(const b2BodyDef* bodyDef)
    {
        return b2CreateBody(get().worldId, bodyDef);
    }

    static void addObject(PhysicsObject* object)
    {
        return get().objects.push_back(object);
    }

    static void removeObject(PhysicsObject* object)
    {
        get()._removeObject(object);
    }

    static void clearObjects()
    {
        get().objects.clear();

        b2DestroyWorld(get().worldId);
        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = {0.0f, 0.0f};
        get().worldId = b2CreateWorld(&worldDef);
    }

    static std::vector<vi2d> getSpawners()
    {
        return get().dungeon.getSpawners();
    }

    static vi2d getStart()
    {
        return { get().dungeon.start.x, get().dungeon.start.y };
    }

    static void doStep(float timeStep)
    {
        b2World_Step(get().worldId, timeStep, 2);
        get()._processContactEvents();
    }

    static b2WorldId getPhysicsWorld()
    {
        return get().worldId;
    }


    static void draw()
    {
        std::vector<PhysicsObject*> tempObjects = get().objects;

        std::sort(tempObjects.begin(), tempObjects.end(), [](PhysicsObject* a, PhysicsObject* b) {
            return b2Body_GetPosition(a->RigidBody).y < b2Body_GetPosition(b->RigidBody).y;
            });

        for (auto const object : tempObjects) {
            if (b2Body_IsValid(object->RigidBody)) {
                object->draw();
            }
        }
    }

    static std::vector<PhysicsObject*> getObjectsOfType(ObjectType type) {
        std::vector<PhysicsObject*> filteredObjects;

        for (auto const object : get().objects) {
            if (object->type == type) {
                filteredObjects.push_back(object);
            }
        }

        return filteredObjects;
    };

    static LerpAnimator* getSlowMotionLerp()
    {
        return get().slowMotionLerp;
    }

    static float getSlowMotionTimer()
    {
        return get().slowMotionTimer;
    }

    static void setSlowMotionTimer(float newValue)
    {
        get().slowMotionTimer = newValue;
    }

    static bool getPlaySlowMotionExit()
    {
        return get().playSlowMotionExit;
    }

    static void setPlaySlowMotionExit(bool newState)
    {
        get().playSlowMotionExit = newState;
    }

    static Dungeon getDungeon() {
        return get().dungeon;
    }

    static Dungeon& getDungeonRef() {
        return get().dungeon;
    }

    static void startSlowMotion()
    {
        if (!get().slowMotionLerp->started) {
            get().playSlowMotionExit = true;
            get().slowMotionTimer = 3.0f;
            get().slowMotionLerp->started = true;
            get().slowMotionLerp->time = 0;
        }
    }


    static void generateNewMap()
    {
        get()._generateNewMap();
    }

    std::vector<PhysicsObject*> objects;

private:
    void _init();
    void _clear();
    void _processContactEvents();

    std::vector<Room*> rooms;

    void _generateNewMap();

    void _removeObject(PhysicsObject* object);

    std::vector<rectf> mergeWalls(const std::vector<Tile>& map, int width, int height);

    Color _backgroundColor = {66, 57, 58, 255};

    LevelManager lm;

    Dungeon dungeon;

    LerpAnimator* slowMotionLerp = nullptr;
    float slowMotionTimer = 0.0f;
    bool playSlowMotionExit = false;

    b2WorldId worldId;

public:
    World(const World&) = delete;
    static World& get() { static World instance; return instance; }

private:
    World()
    {
        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = {0.0f, 0.0f};
        worldId = b2CreateWorld(&worldDef);

        slowMotionLerp = Lerp::getLerp("SlowMoLerp", 1.0f, 2.0f, 0.5f);
        slowMotionLerp->started = false;
    }
};


