#pragma once

#include <vector>
#include <optional>
#include <functional>
#include <unordered_map>
#include <cstdint>

#include "ecs.h"
#include "world/world.h"
#include "components/rigidbody2d.h"
#include "components/pushable.h"
#include "map/room.h"

// ---------------------------------------------------------------------------
// Beam color — bitmask of R/G/B channels
// ---------------------------------------------------------------------------
using BeamColor = uint8_t;
namespace BeamColors {
    constexpr BeamColor None    = 0;
    constexpr BeamColor Red     = 1;
    constexpr BeamColor Green   = 2;
    constexpr BeamColor Blue    = 4;
    constexpr BeamColor Yellow  = Red | Green;
    constexpr BeamColor Magenta = Red | Blue;
    constexpr BeamColor Cyan    = Green | Blue;
    constexpr BeamColor White   = Red | Green | Blue;
}

inline Color beamToColor(BeamColor c) {
    switch (c) {
        case BeamColors::Red:     return { 255,  60,  60, 255 };
        case BeamColors::Green:   return {  60, 210,  60, 255 };
        case BeamColors::Blue:    return {  80, 140, 255, 255 };
        case BeamColors::Yellow:  return { 255, 220,  40, 255 };
        case BeamColors::Magenta: return { 220,  60, 220, 255 };
        case BeamColors::Cyan:    return {  60, 210, 210, 255 };
        default:                  return { 255, 240, 200, 255 }; // white / mixed
    }
}

// ---------------------------------------------------------------------------
// ECS Components
// ---------------------------------------------------------------------------

struct MirrorBlock {
    enum class Orientation { Slash = 0, Backslash = 1 };
    enum class Rail        { None = 0, Horizontal, Vertical };
    Orientation orientation = Orientation::Slash;
    Rail        rail        = Rail::None;
    int         railMin     = 0;
    int         railMax     = 0;
    int         roomId      = -1;
};

struct BeamEmitter {
    int       roomId    = -1;
    vi2d      tilePos   = {};
    vi2d      direction = {};
    BeamColor color     = BeamColors::White;
};

struct BeamReceptor {
    int       roomId        = -1;
    vi2d      tilePos       = {};
    bool      isLit         = false;
    BeamColor requiredColor = BeamColors::White;
    BeamColor currentColor  = BeamColors::None;
};

struct BeamBlocker {
    int  roomId  = -1;
    vi2d tilePos = {};
};

// Prism splitter: white beam enters from base (opposite of facing).
//   B exits in facing direction (tip).
//   R exits leftOf(facing)  = { facing.y, -facing.x }
//   G exits rightOf(facing) = { -facing.y, facing.x }
// Beams entering from non-base directions pass through unaffected.
struct BeamSplitter {
    int  roomId  = -1;
    vi2d tilePos = {};
    vi2d facing  = { 1, 0 }; // which direction the tip points
};

// Absorbs any beam entering it; accumulates colors; re-emits combined in outputDir.
struct BeamCombiner {
    int       roomId      = -1;
    vi2d      tilePos     = {};
    vi2d      outputDir   = { 0, 1 };
    BeamColor accumulated = BeamColors::None;
};

struct BeamSegment {
    vf2d      start = {}, end = {};
    BeamColor color = BeamColors::White;
};

// ---------------------------------------------------------------------------
// Pure puzzle data (no ECS / dungeon dependency)
// ---------------------------------------------------------------------------
struct MirrorPuzzleData {
    struct Mirror {
        vi2d                     pos;
        MirrorBlock::Orientation ori;
        MirrorBlock::Rail        rail    = MirrorBlock::Rail::None;
        int                      railMin = 0; // absolute bounds on constrained axis
        int                      railMax = 0;
    };
    struct Receptor {
        vi2d      pos;
        BeamColor requiredColor = BeamColors::White;
    };
    struct Splitter {
        vi2d              pos;
        vi2d              facing;
        MirrorBlock::Rail rail    = MirrorBlock::Rail::None;
        int               railMin = 0;
        int               railMax = 0;
    };
    struct Combiner {
        vi2d              pos;
        vi2d              outputDir;
        MirrorBlock::Rail rail    = MirrorBlock::Rail::None;
        int               railMin = 0;
        int               railMax = 0;
    };

    struct Emitter {
        vi2d      tilePos;
        vi2d      direction;
        BeamColor color = BeamColors::White;
    };

    int rx0 = 0, rx1 = 0, ry0 = 0, ry1 = 0;
    int px0 = 0, px1 = 0, py0 = 0, py1 = 0;

    std::vector<Emitter>         emitters;
    std::vector<Mirror>          mirrors;
    std::vector<Mirror>          solutionMirrors;
    std::vector<Receptor>        receptors;
    std::vector<Splitter>        splitters;
    std::vector<Splitter>        solutionSplitters;
    std::vector<Combiner>        combiners;
    std::vector<Combiner>        solutionCombiners;
    std::vector<std::pair<int,int>> barrierTiles;
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
inline vi2d reflectBeam(vi2d dir, MirrorBlock::Orientation ori) {
    return (ori == MirrorBlock::Orientation::Slash)
        ? vi2d{ -dir.y, -dir.x }
        : vi2d{  dir.y,  dir.x };
}

// leftOf(d) = rotate 90° right in screen space (y-down), gives the "left hand" direction
inline vi2d leftOf (vi2d d) { return {  d.y, -d.x }; }
inline vi2d rightOf(vi2d d) { return { -d.y,  d.x }; }

// ---------------------------------------------------------------------------
// Beam trace context — built once per frame, shared across all emitter traces
// ---------------------------------------------------------------------------
inline int64_t tileKey(int x, int y) { return ((int64_t)x << 32) | (uint32_t)y; }

struct BeamTraceContext {
    std::unordered_map<int64_t, MirrorBlock::Orientation> mirrors;
    std::unordered_map<int64_t, vi2d>                     splitters;
    std::unordered_map<int64_t, vi2d>                     combiners;
    std::unordered_map<int64_t, flecs::entity>             receptors;
};

void BuildBeamTraceContext(BeamTraceContext& ctx);

// ---------------------------------------------------------------------------
// Functions
// ---------------------------------------------------------------------------
std::vector<BeamSegment> traceBeam(vi2d emitterTile, vi2d direction, BeamColor color, Dungeon dg, BeamTraceContext& ctx);

std::optional<MirrorPuzzleData> GenerateMirrorPuzzle(
    int rx0, int rx1, int ry0, int ry1,
    const std::function<bool(int,int)>& isWalkable = {});

void CreateMirrorBlockEntity (vf2d tilePos, MirrorBlock::Orientation ori, MirrorBlock::Rail rail, int railMin, int railMax, int roomId);
void CreateBeamEmitterEntity (vi2d tilePos, vi2d direction, BeamColor color, int roomId);
void CreateBeamReceptorEntity(vi2d tilePos, BeamColor requiredColor, int roomId);
void CreateBeamBlockerEntity (vi2d tilePos, int roomId);
void CreateBeamSplitterEntity(vi2d tilePos, vi2d facing, int roomId);
void CreateBeamCombinerEntity(vi2d tilePos, vi2d outputDir, int roomId);
void SpawnMirrorPuzzleForRoom(Room* room);
