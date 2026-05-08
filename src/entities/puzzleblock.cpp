#include "puzzleblock.h"
#include <luminoveau.h>
#include "utils/helpers.h"
#include <set>
#include <map>
#include <utility>
#include <numeric>
#include <algorithm>

void CreatePuzzleBlockEntity(vf2d tilePos, int colorIdx, int roomId, bool spawnFadeIn) {
    auto& world = ECS::getWorld();
    auto entity = world.entity();

    auto userData = std::make_unique<UserData>();
    userData->entity_id = entity.id();

    b2BodyDef bodyDef = b2DefaultBodyDef();
    // Kinematic: solid collision with the player but cannot be moved by physics forces.
    // All movement is driven explicitly via b2Body_SetTransform.
    bodyDef.type     = b2_kinematicBody;
    bodyDef.position = { tilePos.x + 0.5f, tilePos.y + 0.5f };
    bodyDef.userData = userData.get();
    b2BodyId bodyId  = World::createBody(&bodyDef);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.enableContactEvents = true;
    b2Polygon box = b2MakeBox(0.45f, 0.45f); // slightly larger hitbox for better feel
    b2CreatePolygonShape(bodyId, &shapeDef, &box);

    PuzzleBlock pb;
    pb.roomId         = roomId;
    pb.colorIdx       = colorIdx;
    pb.initialTilePos = { (int)tilePos.x, (int)tilePos.y };

    Pushable push;
    if (spawnFadeIn) {
        pb.spawningIn    = true;
        pb.spawnFadeTimer = 0.f;
        push.settled      = false; // not interactable until fully faded in
    }

    entity.emplace<RigidBody2D>(bodyId)
          .set<PuzzleBlock>(pb)
          .set<Pushable>(push)
          .set<UserDataComponent>({ std::move(userData) });
}

// ---------------------------------------------------------------------------
// Puzzle generator
//
// How it works:
//   1. Place BLOCKS_PER_COLOR blocks per colour, one at a time, inside a
//      central zone.  Each placement is rejected if it would create a premature
//      3-in-a-row for that colour.  This lets block counts be arbitrary (6, 9,
//      10 …) — the player simply needs multiple sequential clears to empty a
//      colour group.
//
//   2. After all colours are placed, do K "scatter" moves to spread blocks
//      away from the zone centre.  Each scatter move is accepted only when it
//      does NOT create a 3-in-a-row for the moved block's colour.
//
// Solvability: any distribution of same-coloured blocks in a large open room
// is solvable — the player pushes them into lines repeatedly until all are
// cleared.  Difficulty comes from cross-colour blocking and block count.
// ---------------------------------------------------------------------------

void SpawnPuzzleForRoom(Room* room, std::vector<std::pair<vf2d, int>>* outInitial) {
    auto dg = World::getDungeon();

    struct BlockState { vi2d pos; int colorIdx; };
    std::vector<BlockState> blocks;

    int cx = room->pos.x + room->size.x / 2;
    int cy = room->pos.y + room->size.y / 2;

    // Central zone — wide enough for 3 colours × 6 blocks each
    const int ZONE_HW = 10, ZONE_HH = 6;
    int zoneMinX = cx - ZONE_HW,  zoneMaxX = cx + ZONE_HW - 1;
    int zoneMinY = cy - ZONE_HH,  zoneMaxY = cy + ZONE_HH - 1;

    auto isWalkable = [&](int tx, int ty) -> bool {
        return dg.isWalkable(dg.getTile(tx, ty));
    };

    // Returns true if the blocks currently in `bs` for colour `c` already
    // contain a 3-or-more consecutive run in any row or column.
    auto colorHasMatch = [&](const std::vector<BlockState>& bs, int c) -> bool {
        std::map<int, std::vector<int>> byRow, byCol;
        for (const auto& b : bs) {
            if (b.colorIdx != c) continue;
            byRow[b.pos.y].push_back(b.pos.x);
            byCol[b.pos.x].push_back(b.pos.y);
        }
        auto run3 = [](std::vector<int>& v) -> bool {
            std::sort(v.begin(), v.end());
            for (int i = 0; i < (int)v.size(); ) {
                int j = i + 1;
                while (j < (int)v.size() && v[j] - v[i] == j - i) ++j;
                if (j - i >= 3) return true;
                i = j;
            }
            return false;
        };
        for (auto& [_, row] : byRow) if (run3(row)) return true;
        for (auto& [_, col] : byCol) if (run3(col)) return true;
        return false;
    };

    std::set<std::pair<int,int>> occupied;

    // -----------------------------------------------------------------------
    // Step 1: place BLOCKS_PER_COLOR blocks per colour in the central zone.
    //
    // BLOCKS_PER_COLOR must decompose into groups of 3–5 so the player can
    // clear them.  6 → two 3-in-a-row clears; 9 → three; 5 → one 5-in-a-row.
    // -----------------------------------------------------------------------
    // Block counts that can always be fully cleared:
    //   3 → one 3-in-a-row
    //   4 → one 4-in-a-row
    //   5 → one 5-in-a-row
    //   6 → two 3-in-a-row  (or 3+3, 4+... etc.)
    //   7 → 3+4
    //   8 → 3+5 or 4+4
    //   9 → three 3-in-a-row
    const int COUNT_OPTIONS[] = { 3, 4, 5, 6, 7, 8, 9 };
    const int NUM_OPTIONS = 7;

    for (int colorIdx = 0; colorIdx < PUZZLE_BLOCK_COLOR_COUNT; ++colorIdx) {
        int blocksForColor = COUNT_OPTIONS[Helpers::GetRandomValue(0, NUM_OPTIONS - 1)];
        int placed = 0;
        for (int attempt = 0; placed < blocksForColor && attempt < blocksForColor * 400; ++attempt) {
            int px = Helpers::GetRandomValue(zoneMinX, zoneMaxX);
            int py = Helpers::GetRandomValue(zoneMinY, zoneMaxY);

            if (!isWalkable(px, py))            continue;
            if (occupied.count({px, py}))       continue;

            blocks.push_back({ {px, py}, colorIdx });

            if (colorHasMatch(blocks, colorIdx)) {
                blocks.pop_back();  // would create premature match — reject
                continue;
            }

            occupied.insert({px, py});
            ++placed;
        }
        // If we couldn't place all blocks (very rare), whatever was placed is fine.
    }

    if (blocks.empty()) return;

    // -----------------------------------------------------------------------
    // Step 2: scatter scramble — spread blocks outward from the zone so they
    // don't start in an obvious pre-solved cluster.  Only accepts moves that
    // don't create a premature 3-in-a-row for the moved block's colour.
    // -----------------------------------------------------------------------
    const int K = 12;
    const vi2d DIRS[4] = {{1,0},{-1,0},{0,1},{0,-1}};

    // Scatter zone is larger than the placement zone — blocks can move into
    // the wider room interior during scrambling.
    const int MARGIN = 3;
    int scatterMinX = room->pos.x + MARGIN,  scatterMaxX = room->pos.x + room->size.x - MARGIN - 1;
    int scatterMinY = room->pos.y + MARGIN,  scatterMaxY = room->pos.y + room->size.y - MARGIN - 1;

    int scrambled = 0;
    for (int iter = 0; iter < K * 40 && scrambled < K; ++iter) {
        int bi = Helpers::GetRandomValue(0, (int)blocks.size() - 1);
        int di = Helpers::GetRandomValue(0, 3);
        vi2d dest = { blocks[bi].pos.x + DIRS[di].x, blocks[bi].pos.y + DIRS[di].y };

        if (!isWalkable(dest.x, dest.y))           continue;
        if (occupied.count({dest.x, dest.y}))      continue;
        if (dest.x < scatterMinX || dest.x > scatterMaxX) continue;
        if (dest.y < scatterMinY || dest.y > scatterMaxY) continue;

        // Tentatively move and check for premature match on this colour only
        vi2d orig = blocks[bi].pos;
        blocks[bi].pos = dest;
        bool match = colorHasMatch(blocks, blocks[bi].colorIdx);
        blocks[bi].pos = orig;
        if (match) continue;

        // Commit
        occupied.erase({orig.x, orig.y});
        blocks[bi].pos = dest;
        occupied.insert({dest.x, dest.y});
        ++scrambled;
    }

    // -----------------------------------------------------------------------
    // Step 3: spawn entities.
    // -----------------------------------------------------------------------
    for (const auto& b : blocks) {
        vf2d fp = { (float)b.pos.x, (float)b.pos.y };
        if (outInitial) outInitial->push_back({ fp, b.colorIdx });
        CreatePuzzleBlockEntity(fp, b.colorIdx, room->id);
    }
}
