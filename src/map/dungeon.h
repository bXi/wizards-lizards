#pragma once

#include <random>
#include <vector>
#include <iostream>

#include "newdungeon.h"
#include "tile.h"

#include "utils/vectors.h"
#include "utils/colors.h"

#include "utils/helpers.h"

struct Position {
    int x, y;
};



struct DungeonTileData {
    int width = 0, height = 0;

    std::vector<int> tiles;
};

class Dungeon {

    DungeonTileData data;

public:
    Position start = {-1, -1};

    void copyStringToTiles(const std::string &inputMap) {
        for (char c: inputMap) {
            _tiles.push_back(static_cast<Tile>(c));
        }

    }


public:
    void generate(int maxFeatures) {

        DungeonGen dg;

        dg.generateLayout();

        _width = dg.getSize().x * dg.totalRoomSize;
        _height = dg.getSize().y * dg.totalRoomSize;

        std::string newMap = dg.getMapString();
        copyStringToTiles(newMap);

        for (auto &room: dg.getRooms()) {
            room->pos = room->pos * dg.totalRoomSize;
            room->size = room->size * dg.totalRoomSize;

            if (room->start) {
                start.x = room->pos.x + dg.totalRoomSize / 2;
                start.y = room->pos.y + dg.totalRoomSize / 2;
            }

        }

        if (start.x < 0) start.x = start.y = 9;

        _rooms = dg.getRooms();

        setDungeonTileData();
    }

    void getTestRoom() {
        start.x = 34;
        start.y = 19;

        _width = 80;
        _height = 50;
        _rooms.push_back(new Room("Debug room!", {_width, _height}));

        std::string debugMap = ""
                               "##########......................................................................"
                               "#        #..........#######....................................................."
                               "#        #..........#     #....................................................."
                               "#        #........### ### ###...........####.####..............................."
                               "#        #........#   # #   #...........#  #.#  #..............................."
                               "##########........# ### ### #...........# ##### #..............................."
                               "..................# #     # #...........###   ###..............................."
                               "..................# ### ### #.............#   #................................."
                               "..................#   # #   #...........###   ###..............................."
                               "..................### ### ###...........# ##### #..............................."
                               "....................#     #.............#  #.#  #..............................."
                               "....................#######.............####.####...............#####..........."
                               "......................................................#####.....#   #..........."
                               "......................................................#   #.....# # #..........."
                               "....................####################################  #..#### # ####........"
                               "....................#  #   #             # #  #        ####..#    #    #........"
                               "....................# ###  # #####+##### ## ####       #..#### ####### #........"
                               "....................#      # #         # #    #        #..# ##    #    #........"
                               "....................# ###### #         # #      #      #### ##### # ####........"
                               "....................#  #   # +  >   <  # #      #         # #...# # #..........."
                               "....................######## #         # #   ### ###   ######...#   #..........."
                               "....................#      # #         # ##     #      #.# #....#####..........."
                               "....................#    ### ########+## # #    #      #.# #...................."
                               "....................#    # #             #  #          #####...................."
                               "....................#    # ##########+##################  #.#####..............."
                               "....................# ######                          #   #.# # #..............."
                               "....................#      #                          #####.# # #..............."
                               "....................#      # #  # #                       #######..............."
                               "....................#      # # ## ##                         # #................"
                               "....................#        # #   #                      #### #................"
                               "....................#        # #   #                      #..# #................"
                               "....................#                 #                   #..###................"
                               "....................#                 #                   #....................."
                               "....................###################                   #....................."
                               "......................................#####################....................."
                               "................................................................................"
                               "................................................................................"
                               "................................................................................"
                               "................................................................................"
                               "................................................................................"
                               "................................................................................"
                               "................................................................................"
                               "................................................................................"
                               "................................................................................"
                               "................................................................................"
                               "................................................................................"
                               "................................................................................"
                               "................................................................................"
                               "................................................................................"
                               "................................................................................";
        copyStringToTiles(debugMap);

    }

    void getBossRoom() {
        start.x = 10; // 63;
        start.y = 10; // 63;

        _width = 128;
        _height = 128;
        _rooms.push_back(new Room("Boss room!", {_width, _height}));

        //*
        _spawnpoints.push_back({4, 4});
        _spawnpoints.push_back({8, 4});
        _spawnpoints.push_back({12, 4});
        _spawnpoints.push_back({16, 4});
        _spawnpoints.push_back({20, 4});
        _spawnpoints.push_back({24, 4});
        //*/

        std::string bossRoom = ""
                               "................................................................................................................................"
                               ".##############################################################################################################################."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#  x   x   x                                                                                                              x  #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".#  x                                                                                                                      x  #."
                               ".#                                                                                                                            #."
                               ".#                                                                                                                            #."
                               ".##############################################################################################################################."
                               "................................................................................................................................";
        copyStringToTiles(bossRoom);

    }

    void print() {
        for (int y = 0; y < _height; ++y) {
            for (int x = 0; x < _width; ++x)
                std::cout << static_cast<char>(getTile(x, y));

            std::cout << std::endl;
        }
    }

    int getWidth() {
        return _width;
    }

    int getHeight() {
        return _height;
    }

    std::vector<Tile> getTiles() {
        return _tiles;
    }

    std::vector<vi2d> getSpawners() {
        return _spawnpoints;
    }

    Tile getTile(int x, int y) const {
        if (x < 0 || y < 0 || x >= _width || y >= _height)
            return Tile::VOID;

        const auto tileId = x + y * _width;
        return _tiles[static_cast<int64_t>(tileId)];
    }

    void create(int width, int height) {
        _width = width;
        _height = height;
        _tiles.resize(static_cast<int64_t>(width) * static_cast<int64_t>(height), Tile::VOID);
    }

    void reset() {
        _spawnpoints.clear();
        create(0, 0);
    }

    std::vector<Room *> getRooms() {
        return _rooms;
    }

    bool isWalkable(Tile tile) {

        return tile == Tile::FLOOR || tile == Tile::OPENHDOOR || tile == Tile::OPENVDOOR || tile == Tile::UPSTAIRS || tile == Tile::DOWNSTAIRS;
    };

    void setDungeonTileData()
    {
        data.width = getWidth();
        data.height = getHeight();
        data.tiles.clear();

        for (int y = 0; y < data.height; y++) {
            for (int x = 0; x < data.width; x++) {
                int curTileId = -1;

                Tile UPLEFTTILE = getTile(x - 1, y - 1);
                Tile DOWNLEFTTILE = getTile(x - 1, y + 1);

                Tile UPRIGHTTILE = getTile(x + 1, y - 1);
                Tile DOWNRIGHTTILE = getTile(x + 1, y + 1);

                Tile LEFTTILE = getTile(x - 1, y);
                Tile RIGHTTILE = getTile(x + 1, y);

                Tile UPTILE = getTile(x, y - 1);
                Tile DOWNTILE = getTile(x, y + 1);

                switch (getTile(x, y)) {
                case Tile::VOID: {
                    curTileId = 15;
                }
                               break;
                case Tile::WALL: {
                    int tileId = 0;
                    int floorSurrounding = 0;

                    if (!isWalkable(UPTILE)) tileId += 1;
                    if (!isWalkable(DOWNTILE)) tileId += 2;
                    if (!isWalkable(LEFTTILE)) tileId += 4;
                    if (!isWalkable(RIGHTTILE)) tileId += 8;

                    if (isWalkable(UPLEFTTILE)) floorSurrounding += 1;
                    if (isWalkable(DOWNRIGHTTILE)) floorSurrounding += 2;
                    if (isWalkable(DOWNLEFTTILE)) floorSurrounding += 4;
                    if (isWalkable(UPRIGHTTILE)) floorSurrounding += 8;

                    if (tileId == 5 &&
                        (floorSurrounding == 15 || floorSurrounding == 3 || floorSurrounding == 11))
                        tileId = 32;
                    else if (tileId == 6 &&
                        (floorSurrounding == 15 || floorSurrounding == 12 || floorSurrounding == 14))
                        tileId = 33;
                    else if (tileId == 9 &&
                        (floorSurrounding == 15 || floorSurrounding == 12 || floorSurrounding == 13))
                        tileId = 34;
                    else if (tileId == 10 &&
                        (floorSurrounding == 15 || floorSurrounding == 3 || floorSurrounding == 7))
                        tileId = 35;


                    else if (tileId == 7) {
                        if (floorSurrounding == 1 || floorSurrounding == 9 || floorSurrounding == 11 ||
                            floorSurrounding == 3)
                            tileId = 31;
                        if (floorSurrounding == 14 || floorSurrounding == 6 || floorSurrounding == 12 ||
                            floorSurrounding == 4)
                            tileId = 30;
                        if (floorSurrounding == 15 || floorSurrounding == 5 || floorSurrounding == 7 ||
                            floorSurrounding == 13)
                            tileId = 37;
                    }
                    else if (tileId == 11) {
                        if (floorSurrounding == 2 || floorSurrounding == 7 || floorSurrounding == 6 ||
                            floorSurrounding == 3)
                            tileId = 28;
                        if (floorSurrounding == 8 || floorSurrounding == 9 || floorSurrounding == 12 ||
                            floorSurrounding == 13)
                            tileId = 29;
                        if (floorSurrounding == 15 || floorSurrounding == 10 || floorSurrounding == 11 ||
                            floorSurrounding == 14)
                            tileId = 36;
                    }
                    else if (tileId == 13) {
                        if (floorSurrounding == 1 || floorSurrounding == 7 || floorSurrounding == 5 ||
                            floorSurrounding == 3)
                            tileId = 24;
                        if (floorSurrounding == 8 || floorSurrounding == 14 || floorSurrounding == 12 ||
                            floorSurrounding == 10)
                            tileId = 25;
                        if (floorSurrounding == 15 || floorSurrounding == 9 || floorSurrounding == 11 ||
                            floorSurrounding == 13)
                            tileId = 38;
                    }
                    else if (tileId == 14) {
                        if (floorSurrounding == 4 || floorSurrounding == 13 || floorSurrounding == 5 ||
                            floorSurrounding == 12)
                            tileId = 26;
                        if (floorSurrounding == 2 || floorSurrounding == 11 || floorSurrounding == 10 ||
                            floorSurrounding == 3)
                            tileId = 27;
                        if (floorSurrounding == 7 || floorSurrounding == 14 || floorSurrounding == 15 ||
                            floorSurrounding == 6)
                            tileId = 39;
                    }
                    else if (tileId == 15) {
                        if (floorSurrounding == 15) tileId = 46;

                        if (floorSurrounding == 2) tileId = 16;
                        if (floorSurrounding == 4) tileId = 17;
                        if (floorSurrounding == 8) tileId = 18;
                        if (floorSurrounding == 1) tileId = 19;

                        if (floorSurrounding == 5) tileId = 20;
                        if (floorSurrounding == 9) tileId = 21;
                        if (floorSurrounding == 6) tileId = 22;
                        if (floorSurrounding == 10) tileId = 23;

                        if (floorSurrounding == 14) tileId = 40;
                        if (floorSurrounding == 13) tileId = 43;
                        if (floorSurrounding == 11) tileId = 41;
                        if (floorSurrounding == 7) tileId = 42;

                        if (floorSurrounding == 3) tileId = 45;
                        if (floorSurrounding == 12) tileId = 44;
                    }
                    curTileId = tileId;
                }
                               break;
                case Tile::FLOOR: {
                    int tileId;
                    const int randomFloor = Helpers::GetRandomValue(0, 100);
                    if (randomFloor > 95) {
                        tileId = 56;
                    }
                    else if (randomFloor > 85) {
                        tileId = 57;
                    }
                    else {
                        tileId = 58;
                    }
                    curTileId = tileId;

                }
                                break;
                default:
                    break;
                }

                data.tiles.push_back(curTileId);

            }
        }
    }

    DungeonTileData getDungeonTileData() {
        return data;
        
    }

    void setTile(int x, int y, Tile tile) {
        const auto index = x + y * _width;
        _tiles[static_cast<int64_t>(index)] = tile;
    }

private:


private:
    int _width = 0, _height = 0;
    std::vector<Tile> _tiles;
    std::vector<vi2d> _spawnpoints;
    std::vector<Room *> _rooms;
};