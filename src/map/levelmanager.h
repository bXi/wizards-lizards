#pragma once

#include <mutex>
#include <thread>

#include "luminoveau.h"

#include "map/dungeon.h"
#include "map/room.h"

class LevelManager
{
private:
	TextureAsset finalTexture;
	TextureAsset dungeonTileset;

	TextureAsset finalTextureImage;
public:



	LevelManager();

	Dungeon dungeon;
	
	Dungeon getDungeon();
	Dungeon getBossRoom();
	Dungeon getTestRoom();
	TextureAsset drawLevel();
	void renderRow(int start, int rows, Dungeon dungeon, TextureAsset finalTextureImage, TextureAsset dungeonTileset);

	std::vector<Room*> rooms;

};


