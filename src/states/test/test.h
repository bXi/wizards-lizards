#pragma once

#include "olcUTIL_Geometry2D.h"


#include <vector>
#include <utility>

#include <utils/quadtree.h>
#include <utils/vectors.h>
#include <utils/lerp.h>
#include <utils/colors.h>
#include <utils/helpers.h>
#include <utils/camera.h>

#include "map/newdungeon.h"
#include "entities/mirrorblock.h"

#include <luminoveau.h>

#include "box2d/box2d.h"
#include "box2dobjects.h"
#include "state/state.h"

#include <state/basestate.h>


#include <variant>
using namespace olc::utils::geom2d;

template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

enum class Test
{
	Quadtree,
	Lerp,
	Audio,
	Dungeon,
	LineIntersect,
    Render2D,
	Random,
    J9X_Geom
};

class Testpoint {
public:
	bool intersects = false;
	vf2d pos;

	void draw() {}

	void draw(int x, int y)
	{
		if (intersects)
			Draw::Circle({x + pos.x, y + pos.y}, 2.0f, GREEN);
		else
			Draw::Circle({x + pos.x, y + pos.y}, 2.0f, RED);

	}

};

class TestState : public BaseState {
	int maxWidth;

	std::map<Test, std::string> testList = {
		{Test::Quadtree, "Quadtree"},
		{Test::Lerp, "Lerp"},
		{Test::Audio, "Audio"},
		{Test::Dungeon, "Dungeon"},
		{Test::LineIntersect, "LineIntersect"},
		{Test::Render2D, "Render2D"},
		{Test::Random, "Random"},
        {Test::J9X_Geom, "Javids Geom"},
	};

	std::unordered_map<int, Test> keys = {
		{SDLK_1, Test::Quadtree},
		{SDLK_2, Test::Lerp},
		{SDLK_3, Test::Audio},
		{SDLK_4, Test::Dungeon},
		{SDLK_5, Test::LineIntersect},
		{SDLK_6, Test::Render2D},
		{SDLK_7, Test::Random},
        {SDLK_8, Test::J9X_Geom},
	};

	//quadtree
	std::vector<Testpoint*> testPoints;
	Testpoint* entity = nullptr;
	Test selected = Test::Random;
	int size = 600;
	bool isCircle = false;

	//lerp
	LerpAnimator* testlerp = nullptr;

	//Dungeon
	DungeonGen* dg = nullptr;
	bool showLines = true;

    // Mirror puzzle visualizer
    MirrorPuzzleData puzzleViz;
    bool hasVizPuzzle    = false;
    bool showVizSolution = true;

    struct BlockStats {
        int puzzleCount = 0;
        // Emitters
        int emitRed = 0, emitGreen = 0, emitBlue = 0, emitWhite = 0;
        // Mirrors: slash vs backslash × no-rail / H-rail / V-rail
        int mirSlashNone = 0, mirSlashH = 0, mirSlashV = 0;
        int mirBsNone    = 0, mirBsH    = 0, mirBsV    = 0;
        // Splitters & combiners
        int splitters = 0, combiners = 0;
        // Receptors by required color
        int recRed = 0, recGreen = 0, recBlue = 0;
        int recYellow = 0, recMagenta = 0, recCyan = 0, recWhite = 0;
        // Barrier wall tiles
        int barrierTiles = 0;
    };
    BlockStats blockStats;


    TextureAsset dungeonTileset;

//	Camera cam = { {0.f, 0.f} };

    size_t nSelectedShapeIndex = -1;



olc::vi2d vOldMousePos;

    struct Point
	{
		olc::vf2d points[1]; // the point
	};

	struct Line
	{
		olc::vf2d points[2]; // start, end
	};

	struct Rect
	{
		olc::vf2d points[2]; // pos top left, pos bottom right
	};

	struct Circle
	{
		olc::vf2d points[2]; // center pos, pos on circumference
	};

	struct Triangle
	{
		olc::vf2d points[3]; // the three points
	};

	struct Ray
	{
		olc::vf2d points[2]; // origin, direction
	};

	// Create desired shapes using a sequence of points
	static auto make_internal(const Point& p)    { return p.points[0]; }
	static auto make_internal(const Line& p)     { return line<float>{ p.points[0], p.points[1] }; }
	static auto make_internal(const Rect& p)     { return rect<float>{ p.points[0], (p.points[1] - p.points[0]) }; }
	static auto make_internal(const Circle& p)   { return circle<float>{ p.points[0], (p.points[1]-p.points[0]).mag() }; }
	static auto make_internal(const Triangle& p) { return triangle<float>{ p.points[0], p.points[1], p.points[2] }; }
	static auto make_internal(const Ray& p)      { return ray<float>{ p.points[0], (p.points[1]-p.points[0]).norm() }; }

	// The clever bit (and a bit new to me - jx9)
	using ShapeWrap = std::variant<Point, Line, Rect, Circle, Triangle, Ray>;



	bool CheckOverlaps(const ShapeWrap& s1, const ShapeWrap& s2)
	{
		const auto dispatch = overloads{
			[](const auto& lhs, const auto& rhs)
			{
				return overlaps(make_internal(lhs), make_internal(rhs));
			},

			// Any combination of 'Ray' does not work because 'overlaps' is not implemented for it.
			[](const Ray&, const auto&) { return false; },
			[](const auto&, const Ray&) { return false; },
			[](const Ray&, const Ray&)  { return false; }
		};

		return std::visit(dispatch, s1, s2);
	}

	bool CheckContains(const ShapeWrap& s1, const ShapeWrap& s2)
	{
		const auto dispatch = overloads{
			[](const auto& lhs, const auto& rhs)
			{
				return contains(make_internal(lhs), make_internal(rhs));
			},
			// Any combination of 'Ray' does not work because 'contains' is not implemented for it.
			[](const Ray&, const auto&) { return false; },
			[](const auto&, const Ray&) { return false; },
			[](const Ray&, const Ray&)  { return false; }
		};

		return std::visit(dispatch, s1, s2);
	}

	std::vector<olc::vf2d> CheckIntersects(const ShapeWrap& s1, const ShapeWrap& s2)
	{
		const auto dispatch = overloads{
			[](const auto& lhs, const auto& rhs)
			{
				return intersects(make_internal(lhs), make_internal(rhs));
			},

			// Any combination of 'Ray' does not work because 'intersects' is not implemented for it.
			//[](const Ray&, const auto&) { return std::vector<olc::vf2d>{}; }, - Ray Intersections are implemented - tut tut :P

			// Ray vs Ray - needed explicitly because...
			[](const Ray& lhs, const Ray& rhs)
			{
				return intersects(make_internal(lhs), make_internal(rhs));
			},

			// ...Shape vs Ray - Dont exist but this treats all f(x,ray) as invalid
			[](const auto&, const Ray&) { return std::vector<olc::vf2d>{}; },
		};

		return std::visit(dispatch, s1, s2);
	}

	std::optional<ray<float>> CheckReflect(const olc::utils::geom2d::ray<float>& s1, const ShapeWrap& s2)
	{
		const auto dispatch = overloads{
			[&](const auto& a) -> std::optional<olc::utils::geom2d::ray<float>>
			{
				return reflect(s1, make_internal(a));
			}
		};

		return std::visit(dispatch, s2);
	}

	void draw_internal(const Point& x, const Color col)
	{
		const auto p = make_internal(x);
		Draw::Pixel({(int)p.x, (int)p.y}, col);
	}

	void draw_internal(const Line& x, const Color col)
	{
		const auto l = make_internal(x);
		Draw::Line({l.start.x, l.start.y}, {l.end.x, l.end.y}, col);
	}

	void draw_internal(const Rect& x, const Color col)
	{
		const auto r = make_internal(x);
		Draw::Rectangle({r.pos.x, r.pos.y}, {r.size.x, r.size.y}, col);
	}

	void draw_internal(const Circle& x, const Color col)
	{
		const auto c = make_internal(x);
		Draw::Circle({c.pos.x, c.pos.y}, int32_t(c.radius), col);
	}

	void draw_internal(const Triangle& x, const Color col)
	{
		const auto t = make_internal(x);
		Draw::Triangle({t.pos[0].x, t.pos[0].y}, {t.pos[1].x, t.pos[1].y}, {t.pos[2].x, t.pos[2].y}, col);
	}

	void draw_internal(const Ray& x, const Color col)
	{
		const auto t = make_internal(x);
        auto lend = t.origin+t.direction * 1000.0f;
        Draw::Line({t.origin.x, t.origin.y}, {lend.x, lend.y}, col);

	}

	void DrawShape(const ShapeWrap& shape, const Color col = WHITE)
	{
		std::visit([&](const auto& x)
		{
			draw_internal(x, col);
		}, shape);
	}

    std::vector<ShapeWrap> vecShapes;


public:
	void load() override;
	void unload() override;
	void draw() override;

	void customDrawGrid(int slices, float spacing);

};
