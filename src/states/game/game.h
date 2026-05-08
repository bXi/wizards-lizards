#pragma once

#include <list>
#include <cstring>
#include <set>
#include <map>

#include <utils/helpers.h>
#include <utils/lerp.h>

#include "luminoveau.h"
#include "configuration.h"


#include "entities/spawner.h"


#include "entities/player.h"
#include "entities/lizard.h"
#include "entities/gate.h"
#include "entities/roomsensor.h"
#include "entities/pressureplate.h"
#include "entities/puzzleblock.h"
#include "entities/mirrorblock.h"

#include "world/world.h"
#include "map/room.h"
#include "map/levelmanager.h"

#include <state/basestate.h>
#include "components/damageaccumulator.h"
#include "components/spriteeffect.h"
#include "components/floatingnumber.h"
#include "components/playerxp.h"
#include "components/timewarpdata.h"
#include "components/fragmentdata.h"
#include "components/chainlightningdata.h"
#include "components/whirlwindobjectdata.h"
#include "entities/timewarp.h"
#include "entities/fragment.h"
#include "entities/whirlwindobject.h"
//#include "entities/powerup.h"
inline auto doCirclesOverlap = [](float x1, float y1, float r1, float x2, float y2, float r2)
{
	return fabs((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2)) <= (r1 + r2) * (r1 + r2);
};

class GameState : public BaseState {
private:

	float accumulator = 0;
	float physTime = 1.f; // static_cast<float>(Settings::getMonitorRefreshRate());

	//Music backgroundMusic;

	//Powerup *powerup = nullptr;

	Dungeon dungeon;

	bool startedGameplay = false;

	float speed = 10.0f;

	float screenWidth;
	float screenHeight;

	float screenRatio;
	
	std::vector<Spawner *> spawners;


    int widthMargin = 5, heightMargin = 5;
	TextureAsset currentLevelTexture;

	std::vector<Room*> rooms;
	LerpAnimator* roomTitleLerp = nullptr;

	Spawner *spawner = nullptr;

	LevelManager lm;

	Room* currentRoom = nullptr;
	Room* lastRoom = nullptr;

    // Per-player hue shift effects (P1=no shift, P2=+120°, P3=+240°, P4=+180°)
    ShaderAsset huePassthroughVert;
    ShaderAsset hueShiftFrag;
    EffectAsset playerHueEffects[4];

    // Zelda-style room pan
    bool isPanning = false;
    float panTimer = 0.f;
    static constexpr float PAN_DURATION = 0.45f;
    vf2d panFrom = {};
    vf2d panTo = {};

	bool miniMapVisible = false;
    int miniMapSize = 201;

public:
	void load() override;
	void unload() override;
	void draw() override;

	void setupControls();

	void resetGame();

	void drawUI();

	void handleInput();
	void update();
	void drawEntities();

    void checkPuzzleMatches(int roomId);
    void resetPuzzle(int roomId);
    void checkMirrorPuzzle(int roomId);
    std::set<int> puzzleSolvedRooms;
    std::set<int> mirrorPuzzleSolvedRooms;
    std::map<int, std::vector<std::pair<vf2d, int>>> puzzleInitialState; // roomId → [(tilePos, colorIdx)]
    std::map<int, float> puzzleResetTimers; // roomId → seconds all plates held
    std::map<int, std::vector<BeamSegment>> roomBeamSegments; // rebuilt each frame for rendering

    std::vector<FloatingNumberData> floatingNumbers;

    TextureAsset dungeonTileset;
    DungeonTileData tileData;

    rectf getRectangle(int _x, int _y)
	{
		const rectf rect = {static_cast<float>(_x * Configuration::tileWidth), static_cast<float>(_y * Configuration::tileHeight), static_cast<float>(Configuration::tileWidth), static_cast<float>(Configuration::tileHeight) };
		return rect;
	};

	rectf getTile(int tileId)
	{
		return getRectangle(tileId % 16, tileId / 16);
	};
};
