#pragma once

#include <luminoveau.h>

#include "lizard.h"
#include "physicsobject.h"
#include "map/dungeon.h"
#include "utils/vectors.h"

#include "ui/ui.h"

class Lizard;
class Player;


class Spawner {

public:

	Room* room;
	
	float width = 16.0f;
	float height = 16.0f;

	TextureAsset sprite;

	vf2d pos = { 0.0f , 0.0f };

	float spawnTimer;
	float timeTillNextSpawn = 2.0f;

	int maxSpawns = 100;
	int spawnsLeft = 100;

	void load() 
	{
		width = 32.0f;
		height = 32.0f;
		sprite = AssetHandler::GetTexture("assets/entities/crate.png");

		maxSpawns = 100;
		spawnsLeft = 100;
		spawnTimer = 2.0f;
	}

	void draw() 
	{
        Draw::Texture(sprite, pos * width, {width, height});

        if (spawnTimer < 0.f) spawnTimer = 0.f;

		UI::drawSmallBar(pos, {1, 2}, 28, 6, BLUE, spawnTimer / timeTillNextSpawn);
		UI::drawSmallBar(pos, {1, 12}, 28, 6, GREEN, static_cast<float>(spawnsLeft) / static_cast<float>(maxSpawns));
	}


};