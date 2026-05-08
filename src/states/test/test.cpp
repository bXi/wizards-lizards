#include "test.h"

#include "state/state.h"
#include "box2dobjects.h"
#include <queue>
#include <unordered_set>
#include <algorithm>

#include "utils/vectors.h"
#include "utils/colors.h"

int lastDungeonSize = 0;

#include <ui/ui.h>

b2WorldId testWorldId;

void TestState::load()
{
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {0.0f, 98.0f};
    testWorldId = b2CreateWorld(&worldDef);

    AssetHandler::GetMusic("assets/music/menu.mp3");

    AssetHandler::GetSound("assets/sfx/slowmo-enter.wav");
    AssetHandler::GetSound("assets/sfx/cursor.wav");
    dungeonTileset = AssetHandler::GetTexture("assets/tilesets/dungeon.png");

	//Lerp test
	testlerp = Lerp::getLerp("Testlerp", 400.0f, 400.0f, 3.0f);

	//Quadtree test
	for (int i = 0; i < 200; i++)
	{
		entity = new Testpoint();
		entity->pos.x = static_cast<float>(Helpers::GetRandomValue(0,size));
		entity->pos.y = static_cast<float>(Helpers::GetRandomValue(0, size));
		testPoints.emplace_back(entity);
		entity = nullptr;
	}


	dg = new DungeonGen();

	Camera::SetTarget({((float)(Window::GetWidth() - 300) / 2.0f) + 300 , (float)(Window::GetHeight() - 20)});
    //Camera::SetScale(4.f);

	//ground = BoxObject({ 0, 0 }, { 200, 2 }, BROWN, 0, false);






    vecShapes.push_back({ Point{ { { 550.0f, 10.0f } } } });

		vecShapes.push_back({ Line{ { { 320.0f, 10.0f }, {350.0f, 70.0f} } } });
		vecShapes.push_back({ Line{ { { 380.0f, 10.0f }, {310.0f, 20.0f} } } });

		vecShapes.push_back({ Rect{ { { 380.0f, 10.0f }, {410.0f, 60.0f} } } });

		vecShapes.push_back({ Circle{ { { 430.0f, 20.0f }, {470.0f, 20.0f} } } });
		vecShapes.push_back({ Circle{ { { 630.0f, 300.0f }, {720.0f, 300.0f} } } });
		vecShapes.push_back({ Circle{ { { 630.0f, 300.0f }, {700.0f, 300.0f} } } });

		vecShapes.push_back({ Triangle{{ {350.0f, 100.0f}, {310.0f, 150.0f}, {390.0f, 150.0f}} }});
		vecShapes.push_back({ Triangle{{ {650.0f, 200.0f}, {800.0f, 150.0f}, {750.0f, 400.0f}} }});



}

void TestState::unload()
{
    Audio::StopMusic();
    b2DestroyWorld(testWorldId);
}

void TestState::draw()
{
	if (Input::KeyPressed(SDLK_ESCAPE)) State::SetState("menu");
	{
		constexpr int x = 10;
		int y = 10;

		const char* backText = " ESC: back to menu";
		maxWidth = Text::MeasureText(AssetHandler::GetFont("assets/fonts/APL386.ttf", 18), backText);

		Text::DrawText(AssetHandler::GetFont("assets/fonts/APL386.ttf", 18), {x, Window::GetHeight() - 30.f},backText, WHITE);

#ifndef MSVC
//        std::reverse(testList);
#endif

		for (const auto& item : testList)
		{
			const auto name = Helpers::TextFormat("%3u: %s", static_cast<int>(item.first) + 1, item.second.c_str());
			Text::DrawText(AssetHandler::GetFont("assets/fonts/APL386.ttf", 18), {(float)x, (float)y } , name, (item.first == selected) ? YELLOW : WHITE);
			maxWidth = std::max(maxWidth, Text::MeasureText(AssetHandler::GetFont("assets/fonts/APL386.ttf", 18), name));

            y += 24;
		}

		Draw::Line({maxWidth + 40.f, 0}, {maxWidth + 40.f, (float)Window::GetHeight()}, WHITE);
	}
	for (const auto &key : keys)
	{
		if (Input::KeyPressed(key.first)) selected = key.second;
	}

	switch (selected)
	{
	case Test::Quadtree:
	{
		if (Input::KeyPressed(SDLK_SPACE)) isCircle = !isCircle;

		const vi2d drawOffset = { 400, 50 };


		if (Input::MouseButtonDown(SDL_BUTTON_LEFT))
		{

			auto x = static_cast<float>(Input::GetMousePosition().x - drawOffset.x);
			auto y = static_cast<float>(Input::GetMousePosition().y - drawOffset.y);

			if (x >= 0.0f && x <= static_cast<float>(size) && y >= 0.0f && y <= static_cast<float>(size))
			{
				entity = new Testpoint();
				entity->pos.x = x;
				entity->pos.y = y;
				testPoints.emplace_back(entity);
				entity = nullptr;
			}
		}

		const rectf rect = {0.0f, 0.0f, static_cast<float>(size), static_cast<float>(size) };

		QuadTree quadtree = QuadTree(rect);

		for (const auto& testpoint : testPoints) {
			quadtree.insert({ testpoint->pos.x, testpoint->pos.y, testpoint });
		}

		int found = 0;

		for (const auto& testpoint : testPoints) {
			std::vector<void*> quadtreeEntities;

			constexpr float rectSize = 100.0f;
			if (isCircle)
			{
				QuadTree::AABBCircle searchRange = { static_cast<float>(Input::GetMousePosition().x), static_cast<float>(Input::GetMousePosition().y), rectSize / 2.0f };
				Draw::Circle({(searchRange._x), (searchRange._y)}, searchRange._r, { 255,255,255,127 });
				searchRange._x -= static_cast<float>(drawOffset.x);
				searchRange._y -= static_cast<float>(drawOffset.y);
				quadtree.query(searchRange, &quadtreeEntities);

			}
			else
			{
				QuadTree::AABB searchRange = { static_cast<float>(Input::GetMousePosition().x) - rectSize / 2.0f, static_cast<float>(Input::GetMousePosition().y) - rectSize / 2.0f, rectSize, rectSize };
				Draw::Rectangle({ searchRange._left, searchRange._top}, {searchRange._width, searchRange._height }, { 255,255,255,127 });
				searchRange._left -= static_cast<float>(drawOffset.x);
				searchRange._top -= static_cast<float>(drawOffset.y);
				quadtree.query(searchRange, &quadtreeEntities);

			}

			testpoint->intersects = false;
			for (const auto& target : quadtreeEntities) {
				auto entity = reinterpret_cast<Testpoint*>(target);
				entity->intersects = true;
			}
			testpoint->draw(drawOffset.x, drawOffset.y);
			found = static_cast<int>(quadtreeEntities.size());
			quadtreeEntities.clear();
		}
		quadtree.draw(drawOffset.x, drawOffset.y, WHITE);
		quadtree.reset();
		Text::DrawText(AssetHandler::GetFont("assets/fonts/APL386.ttf", 18), {(float)drawOffset.x, Window::GetHeight() - 30.f}, Helpers::TextFormat("Points: %u | Found: %u", static_cast<int>(testPoints.size()), found), WHITE);
		Text::DrawText(AssetHandler::GetFont("assets/fonts/APL386.ttf", 18), {300.f, Window::GetHeight() - 30.f}, Helpers::TextFormat("%d", Window::GetFPS()), LIME);

	} break;
	case Test::Lerp:
	{
		Draw::CircleFilled({testlerp->getValue(), 100.f}, 10.0f, PINK);

		if (Input::KeyPressed(SDLK_SPACE))
		{
			testlerp->time = 0.0f;
		}

		if (Input::KeyPressed(SDLK_A))
		{
			testlerp->callback = EaseExpoInOut;
		}
		if (Input::KeyPressed(SDLK_S))
		{
			testlerp->callback = EaseBounceOut;
		}

	} break;
	case Test::Audio:
	{
		if (Input::KeyPressed(SDLK_A))
		{
            Audio::PlaySound(AssetHandler::GetSound("assets/sfx/cursor.wav"));
		}
		if (Input::KeyPressed(SDLK_S))
		{
            Audio::PlaySound(AssetHandler::GetSound("assets/sfx/slowmo-enter.wav"));
		}
		if (Input::KeyPressed(SDLK_D))
		{
			if (Audio::IsMusicPlaying())
			{
                Audio::StopMusic();
			} else
			{
                Audio::PlayMusic(AssetHandler::GetMusic("assets/music/menu.mp3"));
			}
		}
	} break;
	case Test::Dungeon:
	{
		if (Input::KeyPressed(SDLK_A)) {
			dg->generateLayout();
		}

		if (Input::KeyPressed(SDLK_S)) {
			showLines = !showLines;
		}


		const vi2d drawOffset = { 400, 50 };
		vi2d size = dg->getSize();
		int scalefactor = 512 / size.x;


		if (Input::KeyPressed(SDLK_M)) {
			auto mapString = dg->getMapString();

			auto getTile = [&](int x, int y) {
				if (x < 0 || y < 0 || x >= size.x * 18 || y >= size.y * 18)
					return '.';

				const auto tileId = x + (y * size.x * 18);
				return mapString[static_cast<size_t>(tileId)];
			};
			std::cout << std::endl;
			std::cout << std::endl;

			int _height = size.x * 18;
			int _width = size.y * 18;
				for (int y = 0; y < _height; ++y) {
					for (int x = 0; x < _width; ++x)
						std::cout << getTile(x, y);

					std::cout << std::endl;
				}

		}


		Draw::RectangleFilled({(float)drawOffset.x, (float)drawOffset.y}, {(float)size.x * scalefactor, (float)size.y * scalefactor}, WHITE);

		auto rooms = dg->getRooms();

		for (int y = 0; y < size.y; y++) {
			for (int x = 0; x < size.x; x++) {
				
				int xPos = drawOffset.x + (x * scalefactor);
				int yPos = drawOffset.y + (y * scalefactor);

				Draw::RectangleFilled({(float)xPos, (float)yPos}, {(float)scalefactor, (float)scalefactor}, DARKGRAY);
			}
		}

		for (auto& room : rooms) {
			int xPos = drawOffset.x + (room->pos.x * scalefactor);
			int yPos = drawOffset.y + (room->pos.y * scalefactor);

			Color color = WHITE;

			if (room->name == "Start") color = GREEN;
			else if (room->name == "End") color = RED;



			Draw::RectangleFilled({(float)xPos, (float)yPos}, {(float)scalefactor * room->size.x, (float)scalefactor * room->size.y}, BLACK);

			if (room->northExits == 0 && room->southExits == 0 && room->eastExits == 0 && room->westExits == 0) {
				Draw::RectangleFilled({(float)xPos, (float)yPos}, {(float)scalefactor * room->size.x, (float)scalefactor * room->size.y}, DARKRED);

			}

            auto CheckCollisionPointRec = [&](vf2d point, rectf rect){
                return (point.x >= rect.x && point.x <= rect.x + rect.width && point.y >= rect.y && point.y <= rect.y + rect.height);
            };


			if (CheckCollisionPointRec(Input::GetMousePosition(), rectf(static_cast<float>(xPos), static_cast<float>(yPos), static_cast<float>(scalefactor) * room->size.x, static_cast<float>(scalefactor) * room->size.y))) {
				color = PINK;

				Text::DrawText(AssetHandler::GetFont("assets/fonts/APL386.ttf", 18), {1000, 100}, Helpers::TextFormat("ID: %d/%d", room->id, rooms.size()),  LIME);
				Text::DrawText(AssetHandler::GetFont("assets/fonts/APL386.ttf", 18), {1000, 125}, Helpers::TextFormat("North: %d", room->northExits),  LIME);
				Text::DrawText(AssetHandler::GetFont("assets/fonts/APL386.ttf", 18), {1000, 150}, Helpers::TextFormat("South: %d", room->southExits),  LIME);
				Text::DrawText(AssetHandler::GetFont("assets/fonts/APL386.ttf", 18), {1000, 175}, Helpers::TextFormat("East: %d", room->eastExits),  LIME);
				Text::DrawText(AssetHandler::GetFont("assets/fonts/APL386.ttf", 18), {1000, 200}, Helpers::TextFormat("West: %d", room->westExits),  LIME);
			}



			Draw::Rectangle({(float)xPos, (float)yPos}, {(float)scalefactor * room->size.x, (float)scalefactor * room->size.y}, color);
			Draw::Rectangle({xPos + 1.f, yPos + 1.f}, {(scalefactor * room->size.x) - 2.f, (scalefactor * room->size.y) - 2.f}, color);

			int doorSize = scalefactor / 4; // Adjust the size of the doorways
			
			if (room->northExits > 0) {
				int doorX = xPos + (scalefactor / 2) - (doorSize / 2);
				int doorY = yPos;
				int doorBit = 1;

				for (int i = 0; i < room->size.x; i++) {
					if (room->northExits & doorBit) {
                        Draw::RectangleFilled({(float)doorX, (float)doorY}, {(float)doorSize, (float)doorSize}, showLines ? RED : BLACK);
					}
					doorX += scalefactor;
					doorBit <<= 1;
				}
			}

			if (room->southExits > 0) {
				int doorX = xPos + (scalefactor / 2) - (doorSize / 2);
				int doorY = yPos + (room->size.y * scalefactor) - doorSize;
				int doorBit = 1;

				for (int i = 0; i < room->size.x; i++) {
					if (room->southExits & doorBit) {
                        Draw::RectangleFilled({(float)doorX, (float)doorY}, {(float)doorSize, (float)doorSize}, showLines ? YELLOW : BLACK);
					}
					doorX += scalefactor;
					doorBit <<= 1;
				}
			}

			if (room->eastExits > 0) {
				int doorX = xPos + (room->size.x * scalefactor) - doorSize;
				int doorY = yPos + (scalefactor / 2) - (doorSize / 2);
				int doorBit = 1;

				for (int i = 0; i < room->size.y; i++) {
					if (room->eastExits & doorBit) {
                        Draw::RectangleFilled({(float)doorX, (float)doorY}, {(float)doorSize, (float)doorSize}, showLines ? BLUE : BLACK);
					}
					doorY += scalefactor;
					doorBit <<= 1;
				}
			}

			if (room->westExits > 0) {
				int doorX = xPos;
				int doorY = yPos + (scalefactor / 2) - (doorSize / 2);
				int doorBit = 1;

				for (int i = 0; i < room->size.y; i++) {
					if (room->westExits & doorBit) {
                        Draw::RectangleFilled({(float)doorX, (float)doorY}, {(float)doorSize, (float)doorSize}, showLines ? GREEN : BLACK);
					}
					doorY += scalefactor;
					doorBit <<= 1;
				}
			}


			if (showLines) {
				for (auto& edge : dg->edges)
				{
					const auto firstRoom = dg->getRoomById(edge.first.first);
					const auto secondRoom = dg->getRoomById(edge.first.second);

					vf2d firstPoint = (vf2d(firstRoom->size) / 2.f + firstRoom->pos) * static_cast<float>(scalefactor);
					vf2d secondPoint = (vf2d(secondRoom->size) / 2.f + secondRoom->pos) * static_cast<float>(scalefactor);

					Draw::CircleFilled(firstPoint + drawOffset, 3, PINK);
					Draw::CircleFilled(secondPoint + drawOffset, 3, LIME);

					Draw::Line(firstPoint + drawOffset, secondPoint + drawOffset, YELLOW);

				}
			}
		}

		
	} break;
	case Test::LineIntersect:
	{

		const vf2d drawOffset = { 400.0f, 50.0f };

		rectf rect = {100.0f, 100.0f, 400.0f, 400.0f };
		vf2d lineStart = { 300.0f, 300.0f };
		vf2d lineEnd = Input::GetMousePosition();
		lineEnd -= drawOffset;

		rectf drawRect = rect;
		drawRect.x += drawOffset.x;
		drawRect.y += drawOffset.y;

		//DrawRectangleLinesEx(drawRect, 1.0f, WHITE);

        Draw::Rectangle({drawRect.x, drawRect.y}, {drawRect.width, drawRect.height}, WHITE);


		Color lineColor = RED;

		if (Helpers::lineIntersectsRectangle(lineStart, lineEnd, rect))
		{
			lineColor = GREEN;
		}

		//Draw::Line(lineStart + drawOffset, lineEnd + drawOffset, lineColor);

        Draw::Line(lineStart + drawOffset, lineEnd + drawOffset, lineColor);

	} break;

    case Test::Render2D:{

        Draw::Line({300.f, 20.f}, {350.f, 30.f}, WHITE);
        Draw::ThickLine({400.f, 20.f}, {450.f, 30.f}, WHITE, 3.f);


        Draw::Circle({350.f, 100.f}, 40.f, WHITE);
        Draw::CircleFilled({500.f, 100.f}, 40.f, WHITE);

        Draw::Ellipse({350.f, 200.f}, 40.f, 25.f, WHITE);
        Draw::EllipseFilled({500.f, 200.f}, 40.f, 25.f, WHITE);

        vf2d v1 = {40.f, 0.f};
        vf2d v2 = {0.f, 80.f};
        vf2d v3 = {80.f, 80.f};
        Draw::Triangle(v1 + vf2d(300.f, 300.f), v2 + vf2d(300.f, 300.f), v3 + vf2d(300.f, 300.f), WHITE);
        Draw::TriangleFilled(v1 + vf2d(450.5, 300.f), v2 + vf2d(450.5, 300.f), v3 + vf2d(450.5, 300.f), WHITE);

        Draw::Rectangle({300.f, 400.f}, {80.f,80.f}, WHITE);
        Draw::RectangleFilled({450.f, 400.f}, {80.f,80.f}, WHITE);


    } break;
	case Test::Random: {
        auto& font18 = AssetHandler::GetFont("assets/fonts/APL386.ttf", 18);

        if (Input::KeyPressed(SDLK_A)) {
            auto result = GenerateMirrorPuzzle(1, 32, 1, 32);
            if (result) {
                puzzleViz    = *result;
                hasVizPuzzle = true;
                blockStats.puzzleCount++;
                for (const auto& e : result->emitters) {
                    if      (e.color == BeamColors::Red)   blockStats.emitRed++;
                    else if (e.color == BeamColors::Green) blockStats.emitGreen++;
                    else if (e.color == BeamColors::Blue)  blockStats.emitBlue++;
                    else                                   blockStats.emitWhite++;
                }
                for (const auto& m : result->mirrors) {
                    bool slash = (m.ori == MirrorBlock::Orientation::Slash);
                    if      (m.rail == MirrorBlock::Rail::Horizontal) { if (slash) blockStats.mirSlashH++; else blockStats.mirBsH++; }
                    else if (m.rail == MirrorBlock::Rail::Vertical)   { if (slash) blockStats.mirSlashV++; else blockStats.mirBsV++; }
                    else                                               { if (slash) blockStats.mirSlashNone++; else blockStats.mirBsNone++; }
                }
                blockStats.splitters    += (int)result->splitters.size();
                blockStats.combiners    += (int)result->combiners.size();
                for (const auto& r : result->receptors) {
                    switch (r.requiredColor) {
                        case BeamColors::Red:     blockStats.recRed++;     break;
                        case BeamColors::Green:   blockStats.recGreen++;   break;
                        case BeamColors::Blue:    blockStats.recBlue++;    break;
                        case BeamColors::Yellow:  blockStats.recYellow++;  break;
                        case BeamColors::Magenta: blockStats.recMagenta++; break;
                        case BeamColors::Cyan:    blockStats.recCyan++;    break;
                        default:                  blockStats.recWhite++;   break;
                    }
                }
                blockStats.barrierTiles += (int)result->barrierTiles.size();
            }
        }
        if (Input::KeyPressed(SDLK_S) && hasVizPuzzle)
            showVizSolution = !showVizSolution;

        Text::DrawText(font18, {310.f, 10.f}, "A: new puzzle", WHITE);
        Text::DrawText(font18, {310.f, 30.f}, "S: toggle solution / scrambled", WHITE);

        if (!hasVizPuzzle) {
            Text::DrawText(font18, {310.f, 60.f}, "Press A to generate a puzzle.", DARKGRAY);
            break;
        }

        Text::DrawText(font18, {310.f, 60.f},
            showVizSolution ? "VIEW: Solution" : "VIEW: Scrambled", LIME);

        const vf2d off = { 400.f, 200.f };
        const float ts  = 12.f;
        const int   RSZ = 34;

        auto T = [&](float tx, float ty) -> vf2d {
            return { off.x + tx * ts, off.y + ty * ts };
        };

        const auto& pz = puzzleViz;

        Draw::RectangleFilled(T(0, 0), { RSZ * ts, RSZ * ts }, { 18, 18, 18, 255 });
        Draw::RectangleFilled(
            T((float)pz.px0, (float)pz.py0),
            { (pz.px1 - pz.px0 + 1) * ts, (pz.py1 - pz.py0 + 1) * ts },
            { 38, 38, 38, 255 });

        for (const auto& bt : pz.barrierTiles) {
            vf2d bpos = T((float)bt.first, (float)bt.second);
            Draw::RectangleFilled(bpos, { ts, ts }, { 80, 80, 80, 255 });
            Draw::Rectangle(bpos, { ts, ts }, { 160, 160, 160, 200 });
        }

        // Receptors — tinted by required color
        for (const auto& r : pz.receptors) {
            Color rc = beamToColor(r.requiredColor);
            Draw::CircleFilled(T(r.pos.x + 0.5f, r.pos.y + 0.5f), ts * 0.38f, rc);
            Draw::Circle(T(r.pos.x + 0.5f, r.pos.y + 0.5f), ts * 0.38f, WHITE);
        }

        // Helper: draw mirror or splitter diagonal
        auto drawDiag = [&](vf2d tl, bool slash, Color mc, bool halfMirror) {
            Draw::RectangleFilled(tl, { ts, ts }, { mc.r, mc.g, mc.b, halfMirror ? 45u : 60u });
            if (slash)
                Draw::ThickLine({ tl.x + ts, tl.y }, { tl.x, tl.y + ts }, mc, 2.f);
            else
                Draw::ThickLine(tl, { tl.x + ts, tl.y + ts }, mc, 2.f);
            // Extra tick to distinguish splitter from mirror
            if (halfMirror) {
                vf2d cx = { tl.x + ts * 0.5f, tl.y + ts * 0.5f };
                Draw::CircleFilled(cx, ts * 0.12f, mc);
            }
        };

        // Mirrors — with rail indicator
        const auto& activeMirrors = showVizSolution ? pz.solutionMirrors : pz.mirrors;
        for (const auto& m : activeMirrors) {
            vf2d tl = T((float)m.pos.x, (float)m.pos.y);
            bool slash = (m.ori == MirrorBlock::Orientation::Slash);
            Color mc   = slash ? Color{80, 160, 255, 255} : Color{255, 160, 60, 255};
            drawDiag(tl, slash, mc, false);
            // Rail: bounded track rectangle [railMin, railMax] + tick mark on mirror
            if (m.rail == MirrorBlock::Rail::Vertical && m.railMin < m.railMax) {
                vf2d rTL = T((float)m.pos.x, (float)m.railMin);
                vf2d rSZ = { ts, (m.railMax - m.railMin + 1) * ts };
                Draw::RectangleFilled(rTL, rSZ, { mc.r, mc.g, mc.b, 18 });
                Draw::Rectangle(rTL, rSZ, { mc.r, mc.g, mc.b, 80 });
                Draw::ThickLine(tl + vf2d{ts*0.5f, 0.f}, tl + vf2d{ts*0.5f, ts}, mc, 2.f);
            } else if (m.rail == MirrorBlock::Rail::Horizontal && m.railMin < m.railMax) {
                vf2d rTL = T((float)m.railMin, (float)m.pos.y);
                vf2d rSZ = { (m.railMax - m.railMin + 1) * ts, ts };
                Draw::RectangleFilled(rTL, rSZ, { mc.r, mc.g, mc.b, 18 });
                Draw::Rectangle(rTL, rSZ, { mc.r, mc.g, mc.b, 80 });
                Draw::ThickLine(tl + vf2d{0.f, ts*0.5f}, tl + vf2d{ts, ts*0.5f}, mc, 2.f);
            }
        }

        // Splitters — triangular prism: tip = facing direction, R/G/B colored edges
        const auto& activeSplitters = showVizSolution ? pz.solutionSplitters : pz.splitters;
        for (const auto& s : activeSplitters) {
            vf2d tl   = T((float)s.pos.x, (float)s.pos.y);
            // Rail track
            Color sc = { 200, 180, 60, 255 };
            if (s.rail == MirrorBlock::Rail::Vertical && s.railMin < s.railMax) {
                vf2d rTL = T((float)s.pos.x, (float)s.railMin);
                vf2d rSZ = { ts, (s.railMax - s.railMin + 1) * ts };
                Draw::RectangleFilled(rTL, rSZ, { sc.r, sc.g, sc.b, 18 });
                Draw::Rectangle(rTL, rSZ, { sc.r, sc.g, sc.b, 80 });
            } else if (s.rail == MirrorBlock::Rail::Horizontal && s.railMin < s.railMax) {
                vf2d rTL = T((float)s.railMin, (float)s.pos.y);
                vf2d rSZ = { (s.railMax - s.railMin + 1) * ts, ts };
                Draw::RectangleFilled(rTL, rSZ, { sc.r, sc.g, sc.b, 18 });
                Draw::Rectangle(rTL, rSZ, { sc.r, sc.g, sc.b, 80 });
            }
            float half = ts * 0.5f;
            vf2d  c    = tl + vf2d{ half, half };
            vf2d  fd   = { (float)s.facing.x, (float)s.facing.y };
            vf2d  lp   = { fd.y, -fd.x };  // leftOf(facing)
            vf2d  rp   = { -fd.y, fd.x };  // rightOf(facing)
            vf2d  tip  = c + fd * half;
            vf2d  b1   = c + lp * half - fd * half; // top-left of base side
            vf2d  b2   = c + rp * half - fd * half; // bottom-right of base side
            // Fill background
            Draw::RectangleFilled(tl, { ts, ts }, { 30, 30, 30, 200 });
            // Colored edges: Red = b1→tip, Green = b2→tip, Blue = base b1→b2 (input face)
            Draw::ThickLine(b1,  tip, { 255,  60,  60, 255 }, 2.f); // Red
            Draw::ThickLine(b2,  tip, {  60, 210,  60, 255 }, 2.f); // Green
            Draw::ThickLine(b1,  b2,  {  80, 140, 255, 255 }, 2.f); // Blue input base
            Draw::CircleFilled(tip, ts * 0.1f, { 255, 255, 255, 255 });
        }

        // Combiners — X shape
        const auto& activeCombiners = showVizSolution ? pz.solutionCombiners : pz.combiners;
        for (const auto& c : activeCombiners) {
            vf2d tl = T((float)c.pos.x, (float)c.pos.y);
            Color cc = { 60, 220, 200, 255 };
            // Rail track
            if (c.rail == MirrorBlock::Rail::Vertical && c.railMin < c.railMax) {
                vf2d rTL = T((float)c.pos.x, (float)c.railMin);
                vf2d rSZ = { ts, (c.railMax - c.railMin + 1) * ts };
                Draw::RectangleFilled(rTL, rSZ, { cc.r, cc.g, cc.b, 18 });
                Draw::Rectangle(rTL, rSZ, { cc.r, cc.g, cc.b, 80 });
            } else if (c.rail == MirrorBlock::Rail::Horizontal && c.railMin < c.railMax) {
                vf2d rTL = T((float)c.railMin, (float)c.pos.y);
                vf2d rSZ = { (c.railMax - c.railMin + 1) * ts, ts };
                Draw::RectangleFilled(rTL, rSZ, { cc.r, cc.g, cc.b, 18 });
                Draw::Rectangle(rTL, rSZ, { cc.r, cc.g, cc.b, 80 });
            }
            Draw::RectangleFilled(tl, { ts, ts }, { cc.r, cc.g, cc.b, 50 });
            Draw::ThickLine(tl,                       tl + vf2d{ts, ts}, cc, 2.f);
            Draw::ThickLine(tl + vf2d{ts, 0}, tl + vf2d{0, ts}, cc, 2.f);
            // Arrow showing output direction
            vf2d cx = tl + vf2d{ts * 0.5f, ts * 0.5f};
            vf2d tip = cx + vf2d{c.outputDir.x * ts * 0.6f, c.outputDir.y * ts * 0.6f};
            Draw::ThickLine(cx, tip, cc, 2.f);
        }

        // Emitters — filled rect tinted by beam color + direction arrow
        for (const auto& e : pz.emitters) {
            Color ec = beamToColor(e.color);
            vf2d etl = T((float)e.tilePos.x, (float)e.tilePos.y);
            Draw::RectangleFilled(etl, { ts, ts }, { ec.r, ec.g, ec.b, 200 });
            Draw::Rectangle(etl, { ts, ts }, WHITE);
            // Arrow pointing in firing direction
            vf2d ecenter = etl + vf2d{ ts * 0.5f, ts * 0.5f };
            vf2d fd = { (float)e.direction.x, (float)e.direction.y };
            vf2d tip  = ecenter + fd * (ts * 0.45f);
            vf2d base = ecenter - fd * (ts * 0.15f);
            vf2d perp = { -fd.y, fd.x };
            Draw::TriangleFilled(tip,
                base + perp * (ts * 0.2f),
                base - perp * (ts * 0.2f),
                WHITE);
        }
        Draw::Rectangle(T(0, 0), { RSZ * ts, RSZ * ts }, WHITE);

        // Beam trace (solution view — standalone, no ECS)
        if (showVizSolution) {
            std::unordered_map<int64_t, MirrorBlock::Orientation> mirrorMap;
            std::unordered_map<int64_t, vi2d>                     splMap;
            std::unordered_map<int64_t, vi2d>                     comMap;
            std::unordered_set<int64_t>                           recSet;

            for (const auto& m : pz.solutionMirrors)   mirrorMap[tileKey(m.pos.x, m.pos.y)] = m.ori;
            for (const auto& s : pz.solutionSplitters)  splMap[tileKey(s.pos.x, s.pos.y)]   = s.facing;
            for (const auto& c : pz.solutionCombiners)  comMap[tileKey(c.pos.x, c.pos.y)]   = c.outputDir;
            for (const auto& r : pz.receptors)          recSet.insert(tileKey(r.pos.x, r.pos.y));

            struct ActiveBeam { vi2d pos; vi2d dir; BeamColor color; vf2d segStart; };
            std::queue<ActiveBeam> work;
            for (const auto& e : pz.emitters)
                work.push({ e.tilePos, e.direction, e.color,
                    T(e.tilePos.x + 0.5f + e.direction.x * 0.5f,
                      e.tilePos.y + 0.5f + e.direction.y * 0.5f) });

            std::unordered_map<int64_t, BeamColor> combinerAccum;
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
                    pos.x += dir.x; pos.y += dir.y;

                    uint64_t vk = visitKey(pos.x, pos.y, dir.x, dir.y, color);
                    if (visited.count(vk)) break;
                    visited.insert(vk);

                    const bool isWall = std::find(pz.barrierTiles.begin(), pz.barrierTiles.end(), std::make_pair(pos.x, pos.y)) != pz.barrierTiles.end()
                        || pos.x < pz.rx0 || pos.x > pz.rx1
                        || pos.y < pz.ry0 || pos.y > pz.ry1;

                    Color bc = beamToColor(color);
                    vf2d  center = T(pos.x + 0.5f, pos.y + 0.5f);

                    if (isWall) {
                        Draw::ThickLine(segStart, T(pos.x + 0.5f - dir.x * 0.5f, pos.y + 0.5f - dir.y * 0.5f), bc, 2.f);
                        break;
                    }
                    if (recSet.count(tileKey(pos.x, pos.y))) {
                        Draw::ThickLine(segStart, center, bc, 2.f);
                        segStart = center; continue; // beam passes through receptor
                    }
                    auto mirIt = mirrorMap.find(tileKey(pos.x, pos.y));
                    if (mirIt != mirrorMap.end()) {
                        Draw::ThickLine(segStart, center, bc, 2.f);
                        dir = reflectBeam(dir, mirIt->second);
                        segStart = center; continue;
                    }
                    // Prism splitter: beam traveling in facing direction entered through base → split.
                    // Any other direction passes through.
                    auto splIt = splMap.find(tileKey(pos.x, pos.y));
                    if (splIt != splMap.end()) {
                        Draw::ThickLine(segStart, center, bc, 2.f);
                        const vi2d& facing = splIt->second;
                        if (dir.x == facing.x && dir.y == facing.y) {
                            if (color & BeamColors::Blue)
                                work.push({ pos, facing,          BeamColors::Blue,  center });
                            if (color & BeamColors::Red)
                                work.push({ pos, leftOf(facing),  BeamColors::Red,   center });
                            if (color & BeamColors::Green)
                                work.push({ pos, rightOf(facing), BeamColors::Green, center });
                            break;
                        } else {
                            segStart = center; continue;
                        }
                    }
                    auto comIt = comMap.find(tileKey(pos.x, pos.y));
                    if (comIt != comMap.end()) {
                        Draw::ThickLine(segStart, center, bc, 2.f);
                        BeamColor& accum = combinerAccum[tileKey(pos.x, pos.y)];
                        BeamColor newAccum = accum | color;
                        if (newAccum != accum) { accum = newAccum; work.push({ pos, comIt->second, accum, center }); }
                        break;
                    }
                }
            }
        }

        // Legend
        Text::DrawText(font18, {310.f, 85.f},  "/ \\ = mirror (blue=slash, orange=backslash)", { 80, 160, 255, 255 });
        Text::DrawText(font18, {310.f, 105.f}, "triangle = prism splitter (R/G/B faces)",       { 255,  60,  60, 255 });
        Text::DrawText(font18, {310.f, 125.f}, "X = combiner",                                  { 60, 220, 200, 255 });
        Text::DrawText(font18, {310.f, 145.f}, "| = vertical rail  -- = horizontal rail",       { 180, 180, 180, 255 });

        // Block stats panel — bottom-right corner
        if (blockStats.puzzleCount > 0) {
            struct Row { const char* label; int count; Color col; };
            const Color cMirB  = {  80, 160, 255, 255 };
            const Color cMirO  = { 255, 160,  60, 255 };
            const Color cSpl   = { 255,  60,  60, 255 };
            const Color cCom   = {  60, 220, 200, 255 };
            const Row rows[] = {
                { "Emit Red",         blockStats.emitRed,      { 255,  80,  80, 255 } },
                { "Emit Green",       blockStats.emitGreen,    {  80, 210,  80, 255 } },
                { "Emit Blue",        blockStats.emitBlue,     {  80, 140, 255, 255 } },
                { "Emit White",       blockStats.emitWhite,    { 255, 240, 200, 255 } },
                { "Mirror /",         blockStats.mirSlashNone, cMirB },
                { "Mirror \\",        blockStats.mirBsNone,    cMirO },
                { "Mirror / H-rail",  blockStats.mirSlashH,    cMirB },
                { "Mirror \\ H-rail", blockStats.mirBsH,       cMirO },
                { "Mirror / V-rail",  blockStats.mirSlashV,    cMirB },
                { "Mirror \\ V-rail", blockStats.mirBsV,       cMirO },
                { "Splitter",         blockStats.splitters,    cSpl  },
                { "Combiner",         blockStats.combiners,    cCom  },
                { "Recep Red",        blockStats.recRed,       { 255,  80,  80, 255 } },
                { "Recep Green",      blockStats.recGreen,     {  80, 210,  80, 255 } },
                { "Recep Blue",       blockStats.recBlue,      {  80, 140, 255, 255 } },
                { "Recep Yellow",     blockStats.recYellow,    { 255, 220,  40, 255 } },
                { "Recep Magenta",    blockStats.recMagenta,   { 220,  60, 220, 255 } },
                { "Recep Cyan",       blockStats.recCyan,      {  60, 210, 210, 255 } },
                { "Recep White",      blockStats.recWhite,     { 255, 240, 200, 255 } },
                { "Barrier tiles",    blockStats.barrierTiles, { 160, 160, 160, 255 } },
            };
            constexpr int   N      = sizeof(rows) / sizeof(rows[0]);
            constexpr float lineH  = 13.f;
            const float     panelH = lineH * (N + 1) + 8.f;
            const float     panelW = 180.f;
            const float     px     = (float)Window::GetWidth()  - panelW - 10.f;
            const float     py     = (float)Window::GetHeight() - panelH - 10.f;

            Draw::RectangleFilled({ px - 4.f, py - 4.f }, { panelW + 8.f, panelH + 8.f }, { 0, 0, 0, 170 });
            Draw::Rectangle     ({ px - 4.f, py - 4.f }, { panelW + 8.f, panelH + 8.f }, { 80, 80, 80, 200 });

            Text::DrawText(font18, { px, py },
                Helpers::TextFormat("Puzzles: %d", blockStats.puzzleCount), { 200, 200, 200, 255 }, 11.f);

            for (int i = 0; i < N; ++i) {
                float ry = py + lineH * (i + 1) + 4.f;
                Text::DrawText(font18, { px, ry },
                    Helpers::TextFormat("%-18s %d", rows[i].label, rows[i].count), rows[i].col, 10.f);
            }
        }

    } break;
    case Test::J9X_Geom: {
        olc::vf2d vMouseDelta = olc::vf2d(Input::GetMousePosition().x, Input::GetMousePosition().y) - vOldMousePos;
		vOldMousePos = {(int)Input::GetMousePosition().x, (int)Input::GetMousePosition().y};

		if (Input::MouseButtonReleased(SDL_BUTTON_LEFT))
			nSelectedShapeIndex = -1;

		// Check for mouse hovered shapes
		ShapeWrap mouse{ Point{olc::vf2d(Input::GetMousePosition().x, Input::GetMousePosition().y)} };


		if (nSelectedShapeIndex < vecShapes.size() && Input::MouseButtonDown(SDL_BUTTON_LEFT))
		{
			// Visit the selected shape and offset.
			std::visit([&](auto& shape)
			{
				for (auto& p : shape.points)
				{
					p += vMouseDelta;
				}
			}, vecShapes[nSelectedShapeIndex]);
		}

		size_t nMouseIndex = 0;
		for (const auto& shape : vecShapes)
		{
			if (CheckContains(shape, mouse))
			{
				break;
			}

			nMouseIndex++;
		}

		if (nMouseIndex < vecShapes.size() && Input::MouseButtonPressed(SDL_BUTTON_LEFT))
			nSelectedShapeIndex = nMouseIndex;

		// Check Contains
		std::vector<size_t> vContains;
		std::vector<size_t> vOverlaps;
		std::vector<olc::vf2d> vIntersections;
		if (nSelectedShapeIndex < vecShapes.size())
		{
			for (size_t i = 0; i < vecShapes.size(); i++)
			{
				if (i == nSelectedShapeIndex) continue; // No self check

				const auto& vTargetShape = vecShapes[i];

				const auto vPoints = CheckIntersects(vecShapes[nSelectedShapeIndex], vTargetShape);
				vIntersections.insert(vIntersections.end(), vPoints.begin(), vPoints.end());

				if(CheckContains(vecShapes[nSelectedShapeIndex], vTargetShape))
					vContains.push_back(i);

				if (CheckOverlaps(vecShapes[nSelectedShapeIndex], vTargetShape))
					vOverlaps.push_back(i);
			}
		}


		ShapeWrap  ray1, ray2;



		bool bRayMode = false;
		if (Input::MouseButtonDown(SDL_BUTTON_RIGHT))
		{
			// Enable Ray Mode
			bRayMode = true;

			ray1 = { Ray{{ { 10.0f, 10.0f }, olc::vf2d(Input::GetMousePosition().x, Input::GetMousePosition().y)} }};
			ray2 = { Ray{{ { float(Window::GetWidth() - 10), 10.0f }, olc::vf2d(Input::GetMousePosition().x, Input::GetMousePosition().y)} }};


			for (size_t i = 0; i < vecShapes.size(); i++)
			{
				const auto& vTargetShape = vecShapes[i];

				const auto vPoints1 = CheckIntersects(ray1, vTargetShape);
				vIntersections.insert(vIntersections.end(), vPoints1.begin(), vPoints1.end());

				const auto vPoints2 = CheckIntersects(ray2, vTargetShape);
				vIntersections.insert(vIntersections.end(), vPoints2.begin(), vPoints2.end());

			}

			const auto vPoints3 = CheckIntersects(ray2, ray1);
			vIntersections.insert(vIntersections.end(), vPoints3.begin(), vPoints3.end());


		}

		// Draw All Shapes
		for (const auto& shape : vecShapes)
			DrawShape(shape);


		// Draw Overlaps
		for (const auto& shape_idx : vOverlaps)
			DrawShape(vecShapes[shape_idx], YELLOW);

		// Draw Contains
		for (const auto& shape_idx : vContains)
			DrawShape(vecShapes[shape_idx], PURPLE);

		// Draw Manipulated Shape
		if(nSelectedShapeIndex < vecShapes.size())
			DrawShape(vecShapes[nSelectedShapeIndex], GREEN);

		// Draw Intersections
		for (const auto& intersection : vIntersections)
			Draw::CircleFilled({intersection.x, intersection.y}, 3.f, RED);

		if (bRayMode)
		{
			DrawShape(ray1, BLUE);
			DrawShape(ray2, BLUE);
		}

		// Laser beam
		ray<float> ray_laser{ {10.0f, 300.0f}, {1.0f, 0.0f} };
		bool ray_stop = false;
		int nBounces = 100;
		size_t last_hit_index = -1;


		ray<float> ray_reflected;

		while (!ray_stop && nBounces > 0)
		{
			// Find closest
			ray_stop = true;
			int closest_hit_index = -1;
			float fClosestDistance = 10000000.0f;

			for (size_t i = 0; i < vecShapes.size(); i++)
			{
				// Dont check against origin shape
				if (i == last_hit_index) continue;

				const auto& vTargetShape = vecShapes[i];
				auto hit = CheckReflect(ray_laser, vTargetShape);
				if (hit.has_value())
				{
					float d = (ray_laser.origin - hit.value().origin).mag();
					if (d < fClosestDistance)
					{
						fClosestDistance = d;
						closest_hit_index = i;
						ray_reflected = hit.value();
					}
				}
			}

			if (closest_hit_index != -1)
			{
				Draw::Line({ray_laser.origin.x, ray_laser.origin.y} , {ray_reflected.origin.x, ray_reflected.origin.y} , Color(rand() % 155 + 100, 0, 0, 255));
				ray_laser = ray_reflected;
				ray_stop = false;
				last_hit_index = closest_hit_index;
				nBounces--;
			}

			if (ray_stop)
			{
				// Ray didnt hit anything
				nBounces = 0;
                auto target = ray_laser.origin + ray_laser.direction * 1000.0f;
				Draw::Line({ray_laser.origin.x, ray_laser.origin.y}, {target.x, target.y}, Color(rand() % 155 + 100, 0, 0, 255));
			}
		}

    } break;
	}
}

void TestState::customDrawGrid(int slices, float spacing)
{

}
