#include "levelmanager.h"
#include "configuration.h"


LevelManager::LevelManager()
{
	dungeon = Dungeon();
}



Dungeon LevelManager::getDungeon()
{
	dungeon.reset();

	dungeon.generate(0);
	rooms = dungeon.getRooms();

	for (auto& room : rooms) {
		if (room->start || room->end) room->spawners.clear();
	}

	return dungeon;
}


Dungeon LevelManager::getBossRoom()
{
    dungeon.getBossRoom();
	rooms = dungeon.getRooms();
	return dungeon;
}


Dungeon LevelManager::getTestRoom()
{
	dungeon.getTestRoom();
	rooms = dungeon.getRooms();
	return dungeon;
}






void LevelManager::renderRow(const int start, const int rows, Dungeon _dungeon, TextureAsset _finalTextureImage, const TextureAsset _dungeonTileset)
{

	[[maybe_unused]] constexpr char FLOOR = ' ';
	[[maybe_unused]] constexpr char WALL = '#';
	[[maybe_unused]] constexpr char VOID = '.';
	[[maybe_unused]] constexpr char DOOR = '+';
	[[maybe_unused]] constexpr char OPENDOOR = '-';
	[[maybe_unused]] constexpr char UPSTAIRS = '<';
	[[maybe_unused]] constexpr char DOWNSTAIRS = '>';

	auto getRectangle = [&](int _x, int _y)
	{
		const rectf rect = {static_cast<float>(_x * Configuration::tileWidth), static_cast<float>(_y * Configuration::tileHeight), static_cast<float>(Configuration::tileWidth), static_cast<float>(Configuration::tileHeight) };
		return rect;
	};

	auto getTile = [&](int tileId)
	{
		return getRectangle(tileId % 16, tileId / 16);
	};

	auto drawToMap = [&](TextureAsset* _tex, const TextureAsset(_tileset), const rectf tile, const rectf dest, Color color)
	{
		//std::lock_guard<std::mutex> guard(_tex_mutex);
		//ImageDraw(_tex, _tileset, tile, dest, color);
	};

    auto tiles = dungeon.getDungeonTileData();

	for (int x = start; x < start + rows; x++) {
		for (int y = 0; y < tiles.height; y++) {

            int index = y * tiles.width + x;

			Tile UPLEFTTILE = _dungeon.getTile(x - 1, y - 1);
			//Tile DOWNLEFTTILE = _dungeon.GetTile(x - 1, y + 1);

			//Tile UPRIGHTTILE = _dungeon.GetTile(x + 1, y - 1);
			//Tile DOWNRIGHTTILE = _dungeon.GetTile(x + 1, y + 1);

			Tile LEFTTILE = _dungeon.getTile(x - 1, y);
			//Tile RIGHTTILE = _dungeon.GetTile(x + 1, y);

			Tile UPTILE = _dungeon.getTile(x, y - 1);
			//Tile DOWNTILE = _dungeon.GetTile(x, y + 1);


			const rectf destRect = {static_cast<float>(x * Configuration::tileWidth), static_cast<float>(y * Configuration::tileHeight), static_cast<float>(Configuration::tileWidth), static_cast<float>(Configuration::tileHeight) };

            drawToMap(&finalTextureImage, _dungeonTileset, getTile(tiles.tiles.at(index)), destRect, WHITE);

			switch (_dungeon.getTile(x, y)) {
                case Tile::FLOOR:
			{
				if (LEFTTILE == Tile::WALL && UPTILE == Tile::WALL) {
					drawToMap(&_finalTextureImage, _dungeonTileset, getTile(50), destRect, WHITE);
				}
				else if (LEFTTILE == Tile::WALL && UPTILE == Tile::FLOOR && UPLEFTTILE == Tile::WALL) {
					drawToMap(&_finalTextureImage, _dungeonTileset, getTile(52), destRect, WHITE);
				}
				else if (LEFTTILE == Tile::WALL && UPTILE == Tile::FLOOR && dungeon.isWalkable(UPLEFTTILE)) {
					drawToMap(&_finalTextureImage, _dungeonTileset, getTile(51), destRect, WHITE);
				}
				else if (LEFTTILE == Tile::FLOOR && UPTILE == Tile::WALL && UPLEFTTILE == Tile::WALL) {
					drawToMap(&_finalTextureImage, _dungeonTileset, getTile(48), destRect, WHITE);
				}
				else if (LEFTTILE == Tile::FLOOR && UPTILE == Tile::FLOOR && UPLEFTTILE == Tile::WALL) {
					drawToMap(&_finalTextureImage, _dungeonTileset, getTile(53), destRect, WHITE);
				}
				else if (LEFTTILE == Tile::FLOOR && UPTILE == Tile::WALL && UPLEFTTILE == Tile::FLOOR) {
					drawToMap(&_finalTextureImage, _dungeonTileset, getTile(49), destRect, WHITE);
				}
			}
			break;
			default: break;
			}
		}
	}
}

TextureAsset LevelManager::drawLevel()
{
//	dungeonTileset = LoadImage("assets/tilesets/dungeon.png");
//	finalTextureImage = GenImageColor(dungeon.getWidth() * Configuration::tileWidth, dungeon.getHeight() * Configuration::tileHeight, BLACK);
//
//	constexpr int maxThreads = 8;
//
//	std::thread threads[maxThreads];
//
//	for (int i = 0; i < maxThreads; i++) {
//		int rows = dungeon.getWidth() / maxThreads;
//		int start = i * rows;
//		threads[i] = std::thread(&LevelManager::renderRow, this, start, rows, dungeon, finalTextureImage, dungeonTileset);
//	}
//
//	for (size_t i = 0; i < maxThreads; i++)
//		threads[i].join();
//
//	finalTexture = LoadTextureFromImage(finalTextureImage);
//
//	UnloadImage(dungeonTileset);
//	UnloadImage(finalTextureImage);
//
//	return finalTexture;
TextureAsset temp{};
return temp;
}