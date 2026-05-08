#include "mirrorblock.h"
#include "puzzleblock.h"
#include <luminoveau.h>
#include "utils/helpers.h"
#include "map/tile.h"
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <tuple>
#include <algorithm>

// ---------------------------------------------------------------------------
// Entity creation
// ---------------------------------------------------------------------------

static b2BodyId makePushableBody(vf2d tilePos, UserData* ud) {
    b2BodyDef bodyDef   = b2DefaultBodyDef();
    bodyDef.type        = b2_kinematicBody;
    bodyDef.position    = { tilePos.x + 0.5f, tilePos.y + 0.5f };
    bodyDef.userData    = ud;
    b2BodyId bodyId     = World::createBody(&bodyDef);
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.enableContactEvents = true;
    b2Polygon box = b2MakeBox(0.45f, 0.45f);
    b2CreatePolygonShape(bodyId, &shapeDef, &box);
    return bodyId;
}

void CreateMirrorBlockEntity(vf2d tilePos, MirrorBlock::Orientation orientation, MirrorBlock::Rail rail, int railMin, int railMax, int roomId) {
    auto& world = ECS::getWorld();
    auto  entity = world.entity();
    auto  userData = std::make_unique<UserData>();
    userData->entity_id = entity.id();

    b2BodyId bodyId = makePushableBody(tilePos, userData.get());

    MirrorBlock mb;
    mb.roomId      = roomId;
    mb.orientation = orientation;
    mb.rail        = rail;
    mb.railMin     = railMin;
    mb.railMax     = railMax;

    entity.emplace<RigidBody2D>(bodyId)
          .set<MirrorBlock>(mb)
          .set<Pushable>({})
          .set<UserDataComponent>({ std::move(userData) });
}

void CreateBeamEmitterEntity(vi2d tilePos, vi2d direction, BeamColor color, int roomId) {
    BeamEmitter be;
    be.roomId    = roomId;
    be.tilePos   = tilePos;
    be.direction = direction;
    be.color     = color;
    ECS::getWorld().entity().set<BeamEmitter>(be);
}

void CreateBeamReceptorEntity(vi2d tilePos, BeamColor requiredColor, int roomId) {
    BeamReceptor br;
    br.roomId        = roomId;
    br.tilePos       = tilePos;
    br.isLit         = false;
    br.requiredColor = requiredColor;
    br.currentColor  = BeamColors::None;
    ECS::getWorld().entity().set<BeamReceptor>(br);
}

void CreateBeamBlockerEntity(vi2d tilePos, int roomId) {
    auto& world  = ECS::getWorld();
    auto  entity = world.entity();

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type      = b2_staticBody;
    bodyDef.position  = { tilePos.x + 0.5f, tilePos.y + 0.5f };
    bodyDef.userData  = nullptr;
    b2BodyId bodyId   = World::createBody(&bodyDef);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    b2Polygon  box      = b2MakeBox(0.5f, 0.5f);
    b2CreatePolygonShape(bodyId, &shapeDef, &box);

    BeamBlocker bb;
    bb.roomId  = roomId;
    bb.tilePos = tilePos;

    RigidBody2D rb;
    rb.RigidBody = bodyId;
    rb.radius    = 0.f;

    entity.set<RigidBody2D>(rb).set<BeamBlocker>(bb);
}

void CreateBeamSplitterEntity(vi2d tilePos, vi2d facing, int roomId) {
    auto& world    = ECS::getWorld();
    auto  entity   = world.entity();
    auto  userData = std::make_unique<UserData>();
    userData->entity_id = entity.id();

    b2BodyId bodyId = makePushableBody({ (float)tilePos.x, (float)tilePos.y }, userData.get());

    BeamSplitter bs;
    bs.roomId  = roomId;
    bs.tilePos = tilePos;
    bs.facing  = facing;

    entity.emplace<RigidBody2D>(bodyId)
          .set<BeamSplitter>(bs)
          .set<Pushable>({})
          .set<UserDataComponent>({ std::move(userData) });
}

void CreateBeamCombinerEntity(vi2d tilePos, vi2d outputDir, int roomId) {
    auto& world    = ECS::getWorld();
    auto  entity   = world.entity();
    auto  userData = std::make_unique<UserData>();
    userData->entity_id = entity.id();

    b2BodyId bodyId = makePushableBody({ (float)tilePos.x, (float)tilePos.y }, userData.get());

    BeamCombiner bc;
    bc.roomId     = roomId;
    bc.tilePos    = tilePos;
    bc.outputDir  = outputDir;
    bc.accumulated = BeamColors::None;

    entity.emplace<RigidBody2D>(bodyId)
          .set<BeamCombiner>(bc)
          .set<Pushable>({})
          .set<UserDataComponent>({ std::move(userData) });
}

// ---------------------------------------------------------------------------
// Beam trace (ECS-aware, multi-color, supports splitters + combiners)
// ---------------------------------------------------------------------------

void BuildBeamTraceContext(BeamTraceContext& ctx) {
    ctx.mirrors.clear();
    ctx.splitters.clear();
    ctx.combiners.clear();
    ctx.receptors.clear();

    ECS::getWorld().filter<MirrorBlock, Pushable, RigidBody2D>().each([&](flecs::entity, MirrorBlock& mb, Pushable& p, RigidBody2D& rb) {
        b2Vec2 bp = b2Body_GetPosition(rb.RigidBody);
        int tx = p.isMoving ? (int)roundf(p.targetPos.x - 0.5f) : (int)roundf(bp.x - 0.5f);
        int ty = p.isMoving ? (int)roundf(p.targetPos.y - 0.5f) : (int)roundf(bp.y - 0.5f);
        ctx.mirrors[tileKey(tx, ty)] = mb.orientation;
    });

    ECS::getWorld().filter<BeamSplitter, Pushable, RigidBody2D>().each([&](flecs::entity, BeamSplitter& bs, Pushable& p, RigidBody2D& rb) {
        b2Vec2 bp = b2Body_GetPosition(rb.RigidBody);
        int tx = p.isMoving ? (int)roundf(p.targetPos.x - 0.5f) : (int)roundf(bp.x - 0.5f);
        int ty = p.isMoving ? (int)roundf(p.targetPos.y - 0.5f) : (int)roundf(bp.y - 0.5f);
        ctx.splitters[tileKey(tx, ty)] = bs.facing;
    });

    ECS::getWorld().filter<BeamCombiner, Pushable, RigidBody2D>().each([&](flecs::entity, BeamCombiner& bc, Pushable& p, RigidBody2D& rb) {
        b2Vec2 bp = b2Body_GetPosition(rb.RigidBody);
        int tx = p.isMoving ? (int)roundf(p.targetPos.x - 0.5f) : (int)roundf(bp.x - 0.5f);
        int ty = p.isMoving ? (int)roundf(p.targetPos.y - 0.5f) : (int)roundf(bp.y - 0.5f);
        ctx.combiners[tileKey(tx, ty)] = bc.outputDir;
    });

    ECS::getWorld().filter<BeamReceptor>().each([&](flecs::entity e, BeamReceptor& br) {
        ctx.receptors[tileKey(br.tilePos.x, br.tilePos.y)] = e;
    });
}

std::vector<BeamSegment> traceBeam(vi2d emitterTile, vi2d direction, BeamColor emitterColor, Dungeon dg, BeamTraceContext& ctx) {
    std::vector<BeamSegment> segments;

    // local combiner colour accumulation for this trace pass
    std::unordered_map<int64_t, BeamColor> combinerAccum;

    // ---- queue-based multi-path trace ----
    struct ActiveBeam { vi2d pos; vi2d dir; BeamColor color; vf2d segStart; };
    std::queue<ActiveBeam> work;
    work.push({ emitterTile, direction, emitterColor,
        { emitterTile.x + 0.5f + direction.x * 0.5f,
          emitterTile.y + 0.5f + direction.y * 0.5f } });

    std::unordered_set<uint64_t> visited;
    auto visitKey = [](int x, int y, int dx, int dy, BeamColor c) -> uint64_t {
        return ((uint64_t)(uint16_t)x)
             | ((uint64_t)(uint16_t)y << 16)
             | ((uint64_t)(uint8_t)(dx + 1) << 32)
             | ((uint64_t)(uint8_t)(dy + 1) << 34)
             | ((uint64_t)c << 36);
    };

    while (!work.empty()) {
        auto [pos, dir, color, segStart] = work.front();
        work.pop();

        for (int step = 0; step < 200; ++step) {
            pos.x += dir.x;
            pos.y += dir.y;

            uint64_t vk = visitKey(pos.x, pos.y, dir.x, dir.y, color);
            if (visited.count(vk)) break;
            visited.insert(vk);

            // Wall / blocker
            if (!dg.isWalkable(dg.getTile(pos.x, pos.y))) {
                segments.push_back({ segStart,
                    { pos.x + 0.5f - dir.x * 0.5f, pos.y + 0.5f - dir.y * 0.5f }, color });
                break;
            }

            vf2d center = { pos.x + 0.5f, pos.y + 0.5f };

            // Receptor
            auto recIt = ctx.receptors.find(tileKey(pos.x, pos.y));
            if (recIt != ctx.receptors.end()) {
                segments.push_back({ segStart, center, color });
                auto* br = recIt->second.get_mut<BeamReceptor>();
                br->currentColor |= color;
                br->isLit = (br->currentColor == br->requiredColor);
                segStart = center;
                continue;
            }

            // Mirror
            auto mirIt = ctx.mirrors.find(tileKey(pos.x, pos.y));
            if (mirIt != ctx.mirrors.end()) {
                segments.push_back({ segStart, center, color });
                dir = reflectBeam(dir, mirIt->second);
                segStart = center;
                continue;
            }

            // Prism splitter: entering from base (opposite of facing) splits white→R/G/B.
            // Any other entry direction passes through unaffected.
            auto splIt = ctx.splitters.find(tileKey(pos.x, pos.y));
            if (splIt != ctx.splitters.end()) {
                segments.push_back({ segStart, center, color });
                const vi2d& facing = splIt->second;
                if (dir.x == facing.x && dir.y == facing.y) {
                    // beam traveling in facing direction → entered through base → split
                    if (color & BeamColors::Blue)
                        work.push({ pos, facing,          BeamColors::Blue,  center });
                    if (color & BeamColors::Red)
                        work.push({ pos, leftOf(facing),  BeamColors::Red,   center });
                    if (color & BeamColors::Green)
                        work.push({ pos, rightOf(facing), BeamColors::Green, center });
                    break;
                } else {
                    // entered from non-base side → pass through
                    segStart = center;
                    continue;
                }
            }

            // Combiner: absorbs beam, re-emits accumulated colour in output direction
            auto comIt = ctx.combiners.find(tileKey(pos.x, pos.y));
            if (comIt != ctx.combiners.end()) {
                segments.push_back({ segStart, center, color });
                BeamColor& accum    = combinerAccum[tileKey(pos.x, pos.y)];
                BeamColor  newAccum = accum | color;
                if (newAccum != accum) {
                    accum = newAccum;
                    work.push({ pos, comIt->second, accum, center });
                }
                break; // beam absorbed
            }
        }
    }

    return segments;
}

// ---------------------------------------------------------------------------
// Puzzle generator (pure data — no ECS / dungeon dependency)
//
// Four puzzle templates, randomly selected:
//
//  0 SIMPLE_COLOR   — single R/G/B emitter, 1 mirror, 1 receptor
//  1 SPLIT_RGB      — white emitter → prism splitter → R/G/B → 3 receptors
//  2 SPLIT_COMBINE  — white → splitter; R+G combine→Yellow, B→Blue receptor
//  3 TWO_COMBINE    — two colored emitters, mirror routes one into combiner → mixed receptor
//
// No barriers (walls still commented out).
// ---------------------------------------------------------------------------

std::optional<MirrorPuzzleData> GenerateMirrorPuzzle(
    int rx0, int rx1, int ry0, int ry1,
    const std::function<bool(int,int)>& isWalkableFn)
{
    auto walkable = isWalkableFn
        ? isWalkableFn
        : std::function<bool(int,int)>([&](int tx, int ty) {
            return tx >= rx0 && tx <= rx1 && ty >= ry0 && ty <= ry1;
          });

    const int PAD = 4;
    const int px0 = rx0 + PAD, px1 = rx1 - PAD;
    const int py0 = ry0 + PAD, py1 = ry1 - PAD;
    const int PW  = px1 - px0 + 1;
    const int PH  = py1 - py0 + 1;
    if (PW < 6 || PH < 6) return std::nullopt;

    MirrorPuzzleData out;
    out.rx0 = rx0; out.rx1 = rx1; out.ry0 = ry0; out.ry1 = ry1;
    out.px0 = px0; out.px1 = px1; out.py0 = py0; out.py1 = py1;
    // No barriers: out.barrierTiles left empty

    std::vector<bool> occupied(PW * PH, false);
    auto occIdx = [&](int x, int y) { return (y - py0) * PW + (x - px0); };
    auto occGet = [&](int x, int y) -> bool { return occupied[occIdx(x, y)]; };
    auto occSet = [&](int x, int y) { occupied[occIdx(x, y)] = true; };

    // ---- shared helpers ----

    // Returns {railMin, railMax} for a track of random length 4-10 with
    // solAxis placed at a random (non-centered) offset within the track.
    auto makeRailRange = [&](int solAxis, int axisMin, int axisMax) -> std::pair<int,int> {
        int maxLen  = axisMax - axisMin + 1;
        int trackLen = std::max(2, std::min(Helpers::GetRandomValue(4, 10), maxLen));
        int rMinLow  = std::max(axisMin, solAxis - (trackLen - 1));
        int rMinHigh = std::min(solAxis,  axisMax - (trackLen - 1));
        if (rMinLow > rMinHigh) return { axisMin, axisMax };
        int rMin = Helpers::GetRandomValue(rMinLow, rMinHigh);
        return { rMin, rMin + trackLen - 1 };
    };

    auto scrambleMirror = [&](const MirrorPuzzleData::Mirror& sol) -> bool {
        for (int attempt = 0; attempt < 600; ++attempt) {
            int sx, sy;
            if (sol.rail == MirrorBlock::Rail::Vertical) {
                if (sol.railMin >= sol.railMax) return false;
                sx = sol.pos.x;
                sy = Helpers::GetRandomValue(sol.railMin, sol.railMax);
            } else if (sol.rail == MirrorBlock::Rail::Horizontal) {
                if (sol.railMin >= sol.railMax) return false;
                sx = Helpers::GetRandomValue(sol.railMin, sol.railMax);
                sy = sol.pos.y;
            } else { // Rail::None — scramble within 4-tile radius of solution position
                sx = Helpers::GetRandomValue(std::max(px0+1, sol.pos.x-4), std::min(px1-1, sol.pos.x+4));
                sy = Helpers::GetRandomValue(std::max(py0+1, sol.pos.y-4), std::min(py1-1, sol.pos.y+4));
            }
            if (sx == sol.pos.x && sy == sol.pos.y) continue;
            if (!walkable(sx, sy) || occGet(sx, sy)) continue;
            out.mirrors.push_back({ {sx, sy}, sol.ori, sol.rail, sol.railMin, sol.railMax });
            occSet(sx, sy);
            return true;
        }
        return false;
    };

    auto scrambleSplitter = [&](const MirrorPuzzleData::Splitter& sol) -> bool {
        for (int attempt = 0; attempt < 300; ++attempt) {
            int sx = Helpers::GetRandomValue(px0 + 1, px1 - 1);
            int sy = Helpers::GetRandomValue(py0 + 1, py1 - 1);
            if (sx == sol.pos.x && sy == sol.pos.y) continue;
            if (!walkable(sx, sy) || occGet(sx, sy)) continue;
            out.splitters.push_back({ {sx, sy}, sol.facing });
            occSet(sx, sy);
            return true;
        }
        return false;
    };

    auto scrambleCombiner = [&](const MirrorPuzzleData::Combiner& sol) -> bool {
        for (int attempt = 0; attempt < 300; ++attempt) {
            int sx = Helpers::GetRandomValue(px0 + 1, px1 - 1);
            int sy = Helpers::GetRandomValue(py0 + 1, py1 - 1);
            if (sx == sol.pos.x && sy == sol.pos.y) continue;
            if (!walkable(sx, sy) || occGet(sx, sy)) continue;
            out.combiners.push_back({ {sx, sy}, sol.outputDir });
            occSet(sx, sy);
            return true;
        }
        return false;
    };

    auto scrambleAll = [&]() -> bool {
        for (const auto& s : out.solutionMirrors)   if (!scrambleMirror(s))   return false;
        for (const auto& s : out.solutionSplitters) if (!scrambleSplitter(s)) return false;
        for (const auto& s : out.solutionCombiners) if (!scrambleCombiner(s)) return false;
        return true;
    };

    // ---- direction helpers ----
    static const vi2d DIRS4[4] = {{1,0},{-1,0},{0,1},{0,-1}};

    // Steps from (x,y) in direction dir before hitting puzzle boundary (ignores occupied)
    auto edgeSteps = [&](int x, int y, vi2d dir) -> int {
        int n = 0;
        while (true) {
            int nx = x + dir.x*(n+1), ny = y + dir.y*(n+1);
            if (nx < px0 || nx > px1 || ny < py0 || ny > py1) break;
            n++;
        }
        return n;
    };

    // Mirror orientation for beam entering in 'in', exiting in 'out'
    auto oriFor = [](vi2d in, vi2d out) -> MirrorBlock::Orientation {
        return (reflectBeam(in, MirrorBlock::Orientation::Slash) == out)
            ? MirrorBlock::Orientation::Slash : MirrorBlock::Orientation::Backslash;
    };

    // Is position free (walkable, not occupied)?
    auto free = [&](int x, int y) -> bool {
        return walkable(x, y) && !occGet(x, y);
    };

    // Rail type + range for a mirror at (mx,my) that receives beam from beamDir
    auto addMirror = [&](vi2d pos, vi2d beamIn, vi2d beamOut) {
        auto ori = oriFor(beamIn, beamOut);
        if (Helpers::GetRandomValue(0, 2) == 0) { // ~33%: no rail, scrambles within 6 tiles
            out.solutionMirrors.push_back({pos, ori, MirrorBlock::Rail::None, 0, 0});
            return;
        }
        MirrorBlock::Rail rail;
        int rMin, rMax;
        if (beamIn.y != 0) {
            rail = MirrorBlock::Rail::Vertical;
            auto [mn,mx_] = makeRailRange(pos.y, py0+1, py1-1);
            rMin = mn; rMax = mx_;
        } else {
            rail = MirrorBlock::Rail::Horizontal;
            auto [mn,mx_] = makeRailRange(pos.x, px0+1, px1-1);
            rMin = mn; rMax = mx_;
        }
        out.solutionMirrors.push_back({pos, ori, rail, rMin, rMax});
    };

    // ---- procedural generation ----
    //
    // No fixed templates. Each puzzle is grown organically:
    //   1. Place 1-3 emitters at random interior positions with random directions/colors
    //   2. For each emitter beam, grow a path through 1-3 mirrors
    //      White emitters get a splitter → 3 colored branches
    //   3. Each path ends at a receptor
    //   4. Retry until minimum complexity is met (≥2 movable pieces, ≥1 receptor)
    //   5. Scramble movable pieces off their solution positions
    //
    // Extra receptors (4-10 total) are added by the post-processing step below.

    static const BeamColor EMITTER_COLORS[] = {
        BeamColors::Red, BeamColors::Green, BeamColors::Blue, BeamColors::White
    };

    struct ActiveBeam { vi2d pos; vi2d dir; BeamColor color; int bounces; };

    bool generated = false;
    for (int masterAtt = 0; masterAtt < 400 && !generated; ++masterAtt) {
        // Reset state for this attempt
        out = {};
        out.rx0 = rx0; out.rx1 = rx1; out.ry0 = ry0; out.ry1 = ry1;
        out.px0 = px0; out.px1 = px1; out.py0 = py0; out.py1 = py1;
        std::fill(occupied.begin(), occupied.end(), false);

        // ---- Step 1: place emitters ----
        int numEmitters = Helpers::GetRandomValue(1, std::min(3, std::min(PW, PH) / 5 + 1));
        std::queue<ActiveBeam> beamQ;

        bool emittersOk = true;
        for (int i = 0; i < numEmitters && emittersOk; ++i) {
            bool placed = false;
            for (int att = 0; att < 200 && !placed; ++att) {
                vi2d dir = DIRS4[Helpers::GetRandomValue(0, 3)];
                int x = Helpers::GetRandomValue(px0 + 1, px1 - 1);
                int y = Helpers::GetRandomValue(py0 + 1, py1 - 1);
                if (!free(x, y)) continue;
                if (edgeSteps(x, y, dir) < 3) continue; // need room to grow

                BeamColor col = EMITTER_COLORS[Helpers::GetRandomValue(0, 3)];
                // At most one white emitter per puzzle
                for (const auto& e : out.emitters)
                    if (e.color == BeamColors::White) { col = EMITTER_COLORS[Helpers::GetRandomValue(0, 2)]; break; }

                occSet(x, y);
                out.emitters.push_back({{x, y}, dir, col});
                placed = true;
            }
            if (!placed) emittersOk = false;
        }
        if (!emittersOk) continue;

        // ---- Step 1b: force-connect emitter pairs via a shared combiner ----
        // For each pair of emitters, check if their rays intersect at a free cell.
        // If so, place a combiner there — both beams feed into it, the combined
        // output beam continues toward a receptor.  This guarantees every emitter
        // is part of one interconnected network rather than isolated chains.
        {
            std::vector<bool> paired(out.emitters.size(), false);

            for (int i = 0; i < (int)out.emitters.size(); ++i) {
                if (paired[i]) continue;
                for (int j = i + 1; j < (int)out.emitters.size(); ++j) {
                    if (paired[j]) continue;
                    const auto& ea = out.emitters[i];
                    const auto& eb = out.emitters[j];

                    // Walk ea's ray; check if each cell is also on eb's ray
                    int stepsA = edgeSteps(ea.tilePos.x, ea.tilePos.y, ea.direction);
                    for (int k = 2; k <= stepsA; ++k) {
                        vi2d cp = { ea.tilePos.x + ea.direction.x * k,
                                    ea.tilePos.y + ea.direction.y * k };
                        if (!free(cp.x, cp.y)) continue;

                        // Is cp on eb's ray?  Must be a positive multiple of eb.direction ahead of eb.tilePos.
                        int dx = cp.x - eb.tilePos.x, dy = cp.y - eb.tilePos.y;
                        bool onBRay = (eb.direction.x == 0 && dx == 0 && dy * eb.direction.y > 0)
                                   || (eb.direction.y == 0 && dy == 0 && dx * eb.direction.x > 0);
                        if (!onBRay) continue;

                        // Clear path from ea to cp (cells 1..k-1)
                        bool clearA = true;
                        for (int s = 1; s < k && clearA; ++s) {
                            if (!free(ea.tilePos.x + ea.direction.x * s,
                                      ea.tilePos.y + ea.direction.y * s)) clearA = false;
                        }
                        if (!clearA) continue;

                        // Clear path from eb to cp
                        int distB = (eb.direction.x != 0) ? std::abs(dx) : std::abs(dy);
                        bool clearB = true;
                        for (int s = 1; s < distB && clearB; ++s) {
                            if (!free(eb.tilePos.x + eb.direction.x * s,
                                      eb.tilePos.y + eb.direction.y * s)) clearB = false;
                        }
                        if (!clearB) continue;

                        // Pick output direction: perpendicular to ea.direction, away from both inputs
                        vi2d outDir = rightOf(ea.direction);
                        int outSteps = edgeSteps(cp.x, cp.y, outDir);
                        if (outSteps < 2) { outDir = leftOf(ea.direction); outSteps = edgeSteps(cp.x, cp.y, outDir); }
                        if (outSteps < 2) continue;

                        int recDist = Helpers::GetRandomValue(1, std::min(outSteps - 1, 6));
                        vi2d rp = { cp.x + outDir.x * recDist, cp.y + outDir.y * recDist };
                        if (!free(rp.x, rp.y)) continue;

                        // Clear output path (cells 1..recDist-1)
                        bool clearOut = true;
                        for (int s = 1; s < recDist && clearOut; ++s) {
                            if (!free(cp.x + outDir.x * s, cp.y + outDir.y * s)) clearOut = false;
                        }
                        if (!clearOut) continue;

                        // Commit: place combiner + receptor
                        occSet(cp.x, cp.y);
                        occSet(rp.x,  rp.y);
                        BeamColor combined = (BeamColor)(ea.color | eb.color);
                        out.solutionCombiners.push_back({cp, outDir, MirrorBlock::Rail::None, 0, 0});
                        out.receptors.push_back({rp, combined});

                        // Continue the combined beam for extra bounces (more complexity)
                        int extraBounces = Helpers::GetRandomValue(1, 3);
                        beamQ.push({rp, outDir, combined, extraBounces});

                        paired[i] = true;
                        paired[j] = true;
                        break;
                    }
                    if (paired[i]) break;
                }
            }

            // Push unpaired emitters with higher bounce count for difficulty
            for (int i = 0; i < (int)out.emitters.size(); ++i) {
                if (!paired[i]) {
                    int bounces = Helpers::GetRandomValue(2, 4);
                    beamQ.push({out.emitters[i].tilePos, out.emitters[i].direction,
                                out.emitters[i].color, bounces});
                }
            }
        }

        // ---- Step 2: grow beam paths ----
        while (!beamQ.empty()) {
            auto [pos, dir, color, bounces] = beamQ.front();
            beamQ.pop();

            int steps = edgeSteps(pos.x, pos.y, dir);
            if (steps < 1) continue;

            bool extended = false;

            // White beam → try to place splitter
            if (color == BeamColors::White && steps >= 2) {
                for (int att = 0; att < 20 && !extended; ++att) {
                    int dist = Helpers::GetRandomValue(1, steps - 1);
                    vi2d sp = {pos.x + dir.x*dist, pos.y + dir.y*dist};
                    if (!free(sp.x, sp.y)) continue;
                    if (edgeSteps(sp.x, sp.y, leftOf(dir))  < 1) continue;
                    if (edgeSteps(sp.x, sp.y, rightOf(dir)) < 1) continue;
                    if (edgeSteps(sp.x, sp.y, dir)          < 1) continue;
                    occSet(sp.x, sp.y);
                    out.solutionSplitters.push_back({sp, dir});
                    int sub = std::max(0, bounces - 1);
                    beamQ.push({sp, leftOf(dir),  BeamColors::Red,   sub});
                    beamQ.push({sp, rightOf(dir), BeamColors::Green, sub});
                    beamQ.push({sp, dir,          BeamColors::Blue,  sub});
                    extended = true;
                }
                // If splitter can't be placed, fall through → white receptor (not ideal; retry will fix)
            }

            // Colored beam (or white that couldn't split) → try to bounce via mirror
            if (!extended && bounces > 0 && steps >= 2) {
                for (int att = 0; att < 20 && !extended; ++att) {
                    int dist = Helpers::GetRandomValue(1, steps - 1);
                    vi2d mp = {pos.x + dir.x*dist, pos.y + dir.y*dist};
                    if (!free(mp.x, mp.y)) continue;

                    vi2d exits[2] = {leftOf(dir), rightOf(dir)};
                    // Shuffle exit choices
                    if (Helpers::GetRandomValue(0, 1)) std::swap(exits[0], exits[1]);
                    vi2d exitDir = exits[0];
                    if (edgeSteps(mp.x, mp.y, exitDir) < 1) exitDir = exits[1];
                    if (edgeSteps(mp.x, mp.y, exitDir) < 1) continue;

                    occSet(mp.x, mp.y);
                    addMirror(mp, dir, exitDir);
                    beamQ.push({mp, exitDir, color, bounces - 1});
                    extended = true;
                }
            }

            // Terminate: place receptor
            if (!extended) {
                for (int d = steps; d >= 1; --d) {
                    vi2d rp = {pos.x + dir.x*d, pos.y + dir.y*d};
                    if (free(rp.x, rp.y)) {
                        occSet(rp.x, rp.y);
                        out.receptors.push_back({rp, color});
                        break;
                    }
                }
            }
        }

        // ---- Step 3: validate minimum complexity ----
        int movable = (int)(out.solutionMirrors.size() + out.solutionSplitters.size() + out.solutionCombiners.size());
        if (movable < 3 || out.receptors.empty()) continue;

        // ---- Step 4: scramble ----
        if (!scrambleAll()) continue;

        generated = true;
    }

    if (!generated) return std::nullopt;

    // ---- add extra receptors to reach 4-10 target ----
    // Rule: no two receptors of the same color may share a straight beam segment
    // (i.e. there must be at least one mirror/splitter/combiner between same-color receptors).
    {
        int targetCount = Helpers::GetRandomValue(4, 10);

        if ((int)out.receptors.size() < targetCount) {
            // Build solution lookup maps
            std::map<std::pair<int,int>, MirrorBlock::Orientation> solMirrors;
            std::map<std::pair<int,int>, vi2d> solSplitters;
            std::map<std::pair<int,int>, vi2d> solCombiners;
            std::vector<bool> recPositions(PW * PH, false);
            for (const auto& m : out.solutionMirrors)   solMirrors[{m.pos.x, m.pos.y}]   = m.ori;
            for (const auto& s : out.solutionSplitters) solSplitters[{s.pos.x, s.pos.y}] = s.facing;
            for (const auto& c : out.solutionCombiners) solCombiners[{c.pos.x, c.pos.y}] = c.outputDir;
            for (const auto& r : out.receptors)         recPositions[occIdx(r.pos.x, r.pos.y)] = true;

            // Each straight run between interaction points is a "segment" (tagged by id).
            // A (segId, color) pair may host at most one receptor.
            struct BeamCell { vi2d pos; BeamColor color; int segId; };
            std::vector<BeamCell> beamCells;
            std::set<std::pair<int,BeamColor>> usedSegColors; // segments already occupied by a receptor

            int nextSegId = 0;

            struct TraceItem { vi2d pos; vi2d dir; BeamColor color; int segId; };
            std::queue<TraceItem> tq;
            for (const auto& e : out.emitters)
                tq.push({ e.tilePos, e.direction, e.color, nextSegId++ });

            using TState = std::tuple<int,int,int,int,BeamColor>;
            std::set<TState> tvisited;
            std::map<std::pair<int,int>, BeamColor> combAccum;

            while (!tq.empty()) {
                auto [pos, dir, color, segId] = tq.front(); tq.pop();
                for (int step = 0; step < 200; ++step) {
                    pos.x += dir.x; pos.y += dir.y;
                    TState st = { pos.x, pos.y, dir.x, dir.y, color };
                    if (tvisited.count(st)) break;
                    tvisited.insert(st);
                    if (!walkable(pos.x, pos.y)) break;

                    auto key = std::make_pair(pos.x, pos.y);
                    const bool inGrid = pos.x >= px0 && pos.x <= px1 && pos.y >= py0 && pos.y <= py1;

                    // If an existing receptor is here, mark this segment+color as used
                    if (inGrid && recPositions[occIdx(key.first, key.second)]) {
                        usedSegColors.insert({segId, color});
                        continue; // beam passes through
                    }

                    if (inGrid && !occGet(key.first, key.second))
                        beamCells.push_back({ pos, color, segId });

                    auto mirIt = solMirrors.find(key);
                    if (mirIt != solMirrors.end()) {
                        dir   = reflectBeam(dir, mirIt->second);
                        segId = nextSegId++; // reflection starts a new segment
                        continue;
                    }
                    auto splIt = solSplitters.find(key);
                    if (splIt != solSplitters.end()) {
                        const vi2d& facing = splIt->second;
                        if (dir.x == facing.x && dir.y == facing.y) {
                            if (color & BeamColors::Blue)
                                tq.push({ pos, facing,          BeamColors::Blue,  nextSegId++ });
                            if (color & BeamColors::Red)
                                tq.push({ pos, leftOf(facing),  BeamColors::Red,   nextSegId++ });
                            if (color & BeamColors::Green)
                                tq.push({ pos, rightOf(facing), BeamColors::Green, nextSegId++ });
                            break;
                        }
                        continue;
                    }
                    auto comIt = solCombiners.find(key);
                    if (comIt != solCombiners.end()) {
                        BeamColor& accum = combAccum[key];
                        BeamColor newAccum = accum | color;
                        if (newAccum != accum) { accum = newAccum; tq.push({ pos, comIt->second, accum, nextSegId++ }); }
                        break;
                    }
                }
            }

            int extras = targetCount - (int)out.receptors.size();
            for (int i = 0; i < extras && !beamCells.empty(); ++i) {
                // Shuffle pick: find a candidate whose (segId, color) hasn't been used yet
                // Randomize order by picking a random start index and scanning
                int start = Helpers::GetRandomValue(0, (int)beamCells.size() - 1);
                int chosen = -1;
                for (int j = 0; j < (int)beamCells.size(); ++j) {
                    int idx = (start + j) % (int)beamCells.size();
                    if (!usedSegColors.count({beamCells[idx].segId, beamCells[idx].color})) {
                        chosen = idx;
                        break;
                    }
                }
                if (chosen < 0) break; // all remaining segments already have a receptor of that color

                vi2d chosenPos   = beamCells[chosen].pos;
                BeamColor chosenColor = beamCells[chosen].color;
                int chosenSeg    = beamCells[chosen].segId;
                auto posKey = std::make_pair(chosenPos.x, chosenPos.y);

                out.receptors.push_back({ chosenPos, chosenColor });
                occSet(posKey.first, posKey.second);
                recPositions[occIdx(posKey.first, posKey.second)] = true;
                usedSegColors.insert({chosenSeg, chosenColor});

                // Remove all entries for this position
                beamCells.erase(std::remove_if(beamCells.begin(), beamCells.end(),
                    [&](const BeamCell& bc) { return bc.pos.x == chosenPos.x && bc.pos.y == chosenPos.y; }),
                    beamCells.end());
            }
        }
    }

    // ---- cull unsolvable receptors ----
    // Trace all emitters through solution piece positions and remove any receptor
    // whose required color is never actually delivered.  This is a catch-all for
    // generator edge cases (e.g. an extra receptor placed on a beam path segment
    // that requires a combined color but sits on the wrong side of the combiner).
    {
        std::map<std::pair<int,int>, MirrorBlock::Orientation> smV;
        std::map<std::pair<int,int>, vi2d>                     ssV;
        std::map<std::pair<int,int>, vi2d>                     scV;
        for (const auto& m : out.solutionMirrors)   smV[{m.pos.x, m.pos.y}] = m.ori;
        for (const auto& s : out.solutionSplitters) ssV[{s.pos.x, s.pos.y}] = s.facing;
        for (const auto& c : out.solutionCombiners) scV[{c.pos.x, c.pos.y}] = c.outputDir;

        std::map<std::pair<int,int>, BeamColor> recHit;
        for (const auto& r : out.receptors)
            recHit[{r.pos.x, r.pos.y}] = BeamColors::None;

        struct VTI { vi2d pos; vi2d dir; BeamColor color; };
        std::queue<VTI> vtq;
        for (const auto& e : out.emitters)
            vtq.push({ e.tilePos, e.direction, e.color });

        using VState = std::tuple<int,int,int,int,BeamColor>;
        std::set<VState>                        vvis;
        std::map<std::pair<int,int>, BeamColor> vAccum;

        while (!vtq.empty()) {
            auto [pos, dir, color] = vtq.front(); vtq.pop();
            for (int step = 0; step < 200; ++step) {
                pos.x += dir.x; pos.y += dir.y;
                VState st = { pos.x, pos.y, dir.x, dir.y, color };
                if (vvis.count(st)) break;
                vvis.insert(st);
                if (!walkable(pos.x, pos.y)) break;

                auto key = std::make_pair(pos.x, pos.y);
                if (auto it = recHit.find(key); it != recHit.end())
                    it->second |= color; // beam passes through receptor, record delivered color

                if (auto it = smV.find(key); it != smV.end()) { dir = reflectBeam(dir, it->second); continue; }
                if (auto it = ssV.find(key); it != ssV.end()) {
                    const vi2d& f = it->second;
                    if (dir.x == f.x && dir.y == f.y) {
                        if (color & BeamColors::Blue)  vtq.push({ pos, f,            BeamColors::Blue  });
                        if (color & BeamColors::Red)   vtq.push({ pos, leftOf(f),    BeamColors::Red   });
                        if (color & BeamColors::Green) vtq.push({ pos, rightOf(f),   BeamColors::Green });
                        break;
                    }
                    continue;
                }
                if (auto it = scV.find(key); it != scV.end()) {
                    BeamColor& acc = vAccum[key];
                    BeamColor  na  = acc | color;
                    if (na != acc) { acc = na; vtq.push({ pos, it->second, na }); }
                    break;
                }
            }
        }

        out.receptors.erase(
            std::remove_if(out.receptors.begin(), out.receptors.end(),
                [&](const MirrorPuzzleData::Receptor& r) {
                    auto it = recHit.find({r.pos.x, r.pos.y});
                    return it == recHit.end() || (it->second & r.requiredColor) != r.requiredColor;
                }),
            out.receptors.end());
    }

    // ---- add barrier walls ----
    // Build reserved set: everything occupied + all rail range cells + all beam path cells.
    // Walls are only placed in free interior cells that don't touch any reserved cell.
    {
        std::vector<bool> reserved = occupied;

        // Mark all rail range cells as reserved so walls never block piece movement
        auto markRail = [&](const MirrorPuzzleData::Mirror& m) {
            if (m.rail == MirrorBlock::Rail::Horizontal) {
                for (int x = m.railMin; x <= m.railMax; ++x) reserved[occIdx(x, m.pos.y)] = true;
            } else if (m.rail == MirrorBlock::Rail::Vertical) {
                for (int y = m.railMin; y <= m.railMax; ++y) reserved[occIdx(m.pos.x, y)] = true;
            }
        };
        for (const auto& m : out.solutionMirrors) markRail(m);

        // Trace solution beam paths and mark every cell they pass through as reserved
        {
            std::map<std::pair<int,int>, MirrorBlock::Orientation> smMap;
            std::map<std::pair<int,int>, vi2d>                     ssMap;
            std::map<std::pair<int,int>, vi2d>                     scMap;
            for (const auto& m : out.solutionMirrors)   smMap[{m.pos.x, m.pos.y}] = m.ori;
            for (const auto& s : out.solutionSplitters) ssMap[{s.pos.x, s.pos.y}] = s.facing;
            for (const auto& c : out.solutionCombiners) scMap[{c.pos.x, c.pos.y}] = c.outputDir;

            struct WTI { vi2d pos; vi2d dir; BeamColor color; };
            std::queue<WTI> wtq;
            for (const auto& e : out.emitters) wtq.push({ e.tilePos, e.direction, e.color });

            using WState = std::tuple<int,int,int,int,BeamColor>;
            std::set<WState> wvis;
            std::map<std::pair<int,int>, BeamColor> wAccum;

            while (!wtq.empty()) {
                auto [pos, dir, color] = wtq.front(); wtq.pop();
                for (int step = 0; step < 200; ++step) {
                    pos.x += dir.x; pos.y += dir.y;
                    WState st = { pos.x, pos.y, dir.x, dir.y, color };
                    if (wvis.count(st)) break;
                    wvis.insert(st);
                    if (!walkable(pos.x, pos.y)) break;
                    if (pos.x >= px0 && pos.x <= px1 && pos.y >= py0 && pos.y <= py1)
                        reserved[occIdx(pos.x, pos.y)] = true;

                    auto key = std::make_pair(pos.x, pos.y);
                    if (auto it = smMap.find(key); it != smMap.end()) { dir = reflectBeam(dir, it->second); continue; }
                    if (auto it = ssMap.find(key); it != ssMap.end()) {
                        const vi2d& f = it->second;
                        if (dir.x == f.x && dir.y == f.y) {
                            if (color & BeamColors::Blue)  wtq.push({ pos, f,             BeamColors::Blue  });
                            if (color & BeamColors::Red)   wtq.push({ pos, leftOf(f),     BeamColors::Red   });
                            if (color & BeamColors::Green) wtq.push({ pos, rightOf(f),    BeamColors::Green });
                            break;
                        }
                        continue;
                    }
                    if (auto it = scMap.find(key); it != scMap.end()) {
                        BeamColor& acc = wAccum[key];
                        BeamColor na = acc | color;
                        if (na != acc) { acc = na; wtq.push({ pos, it->second, na }); }
                        break;
                    }
                }
            }
        }

        // Mark cells that lie on a raw emitter beam (straight-line, no interactions).
        // A wall is only relevant if a beam would actually hit it when pieces are
        // misplaced — i.e. it's on the unobstructed extension of some emitter's ray.
        // We also trace the straight continuation past each solution mirror (the path
        // the beam takes if that mirror isn't there yet) for the same reason.
        std::vector<bool> onRawBeam(PW * PH, false);
        {
            auto markRay = [&](vi2d start, vi2d dir) {
                vi2d pos = start;
                for (int step = 0; step < 200; ++step) {
                    pos.x += dir.x; pos.y += dir.y;
                    if (!walkable(pos.x, pos.y)) break;
                    if (pos.x < px0 || pos.x > px1 || pos.y < py0 || pos.y > py1) break;
                    if (!reserved[occIdx(pos.x, pos.y)])
                        onRawBeam[occIdx(pos.x, pos.y)] = true;
                }
            };

            // Raw emitter beams: straight from each emitter in its firing direction
            for (const auto& e : out.emitters)
                markRay(e.tilePos, e.direction);

            // Straight continuations past each solution mirror (the path the beam would
            // take if the mirror were absent or in the wrong position)
            for (const auto& m : out.solutionMirrors) {
                for (const vi2d& d : DIRS4)
                    markRay(m.pos, d);
            }
        }

        // Helper: can we place a wall cell here?
        // Must be on a raw beam path — otherwise the wall is invisible to the player
        // (no beam ever reaches it) and feels like random clutter.
        auto canWall = [&](int x, int y) -> bool {
            if (!walkable(x, y)) return false;
            if (x <= px0 || x >= px1 || y <= py0 || y >= py1) return false; // keep border clear
            if (reserved[occIdx(x, y)]) return false;
            if (!onRawBeam[occIdx(x, y)]) return false;
            return true;
        };

        int numWalls = Helpers::GetRandomValue(1, 3);
        for (int w = 0; w < numWalls; ++w) {
            // Pick a random free interior starting cell
            vi2d start = {-1, -1};
            for (int att = 0; att < 200; ++att) {
                int x = Helpers::GetRandomValue(px0 + 1, px1 - 1);
                int y = Helpers::GetRandomValue(py0 + 1, py1 - 1);
                if (canWall(x, y)) { start = {x, y}; break; }
            }
            if (start.x < 0) continue;

            // Choose orientation H or V, then grow a straight segment of 3-5 tiles
            bool horizontal = Helpers::GetRandomValue(0, 1) == 0;
            vi2d step = horizontal ? vi2d{1, 0} : vi2d{0, 1};
            int  len  = Helpers::GetRandomValue(3, 5);

            // Collect candidate tiles, stop if any would be invalid
            std::vector<vi2d> seg;
            seg.push_back(start);
            for (int i = 1; i < len; ++i) {
                vi2d next = { start.x + step.x * i, start.y + step.y * i };
                if (!canWall(next.x, next.y)) break;
                seg.push_back(next);
            }
            if ((int)seg.size() < 3) continue;

            // Commit straight segment
            for (const auto& p : seg) {
                out.barrierTiles.push_back({p.x, p.y});
                reserved[occIdx(p.x, p.y)] = true;
            }

            // Optional L-arm: 1-3 tiles perpendicular from one end
            if (Helpers::GetRandomValue(0, 1)) {
                vi2d armBase = seg.back();
                vi2d armStep = horizontal ? vi2d{0, Helpers::GetRandomValue(0,1) ? 1 : -1}
                                          : vi2d{Helpers::GetRandomValue(0,1) ? 1 : -1, 0};
                int armLen = Helpers::GetRandomValue(1, 3);
                for (int i = 1; i <= armLen; ++i) {
                    vi2d ap = { armBase.x + armStep.x * i, armBase.y + armStep.y * i };
                    if (!canWall(ap.x, ap.y)) break;
                    out.barrierTiles.push_back({ap.x, ap.y});
                    reserved[occIdx(ap.x, ap.y)] = true;
                }
            }
        }
    }

    if (out.emitters.empty() || out.receptors.empty()) return std::nullopt;
    return out;
}

// ---------------------------------------------------------------------------
// Spawn ECS entities from puzzle data
// ---------------------------------------------------------------------------

void SpawnMirrorPuzzleForRoom(Room* room) {
    auto dg = World::getDungeon();

    const int rx0 = room->pos.x + 1, rx1 = room->pos.x + room->size.x - 2;
    const int ry0 = room->pos.y + 1, ry1 = room->pos.y + room->size.y - 2;

    auto result = GenerateMirrorPuzzle(rx0, rx1, ry0, ry1,
        [&](int tx, int ty) { return dg.isWalkable(dg.getTile(tx, ty)); });
    if (!result) return;
    const auto& pd = *result;

    {
        auto& dgRef = World::getDungeonRef();
        for (const auto& bt : pd.barrierTiles) {
            dgRef.setTile(bt.first, bt.second, Tile::WALL);
            CreateBeamBlockerEntity({ bt.first, bt.second }, room->id);
        }
        dgRef.setDungeonTileData();
    }

    for (const auto& e : pd.emitters)
        CreateBeamEmitterEntity(e.tilePos, e.direction, e.color, room->id);

    for (const auto& m : pd.mirrors)
        CreateMirrorBlockEntity({ (float)m.pos.x, (float)m.pos.y }, m.ori, m.rail, m.railMin, m.railMax, room->id);

    for (const auto& r : pd.receptors)
        CreateBeamReceptorEntity(r.pos, r.requiredColor, room->id);

    for (const auto& s : pd.splitters)
        CreateBeamSplitterEntity(s.pos, s.facing, room->id);

    for (const auto& c : pd.combiners)
        CreateBeamCombinerEntity(c.pos, c.outputDir, room->id);
}
