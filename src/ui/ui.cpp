#include "ui.h"
#include <luminoveau.h>
#include <cmath>
#include "configuration.h"
#include "log.h"

#include "components/rigidbody2d.h"
#include "components/playerinput.h"
#include "components/playerclass.h"
#include "components/playerindex.h"
#include "components/playerxp.h"

void UI::drawSmallBar(const vf2d &pos, const vi2d &offset, const int width, const int height, const Color color,
                      const float percentage) {
    const vf2d localOffset = offset;

    const auto tileWidth = static_cast<float>(Configuration::tileWidth);
    const auto tileHeight = static_cast<float>(Configuration::tileHeight);

    Draw::RectangleFilled({((pos.x * tileWidth) + localOffset.x    ), ((pos.y * tileHeight) + localOffset.y    )}, {(float)width,                    (float)height},         WHITE);
    Draw::RectangleFilled({((pos.x * tileWidth) + localOffset.x + 1), ((pos.y * tileHeight) + localOffset.y + 1)}, {(float)width - 2,              (float)height - 2}, BLACK);
    Draw::RectangleFilled({((pos.x * tileWidth) + localOffset.x + 1), ((pos.y * tileHeight) + localOffset.y + 1)}, {(float)(width - 2) * percentage, (float)height - 2}, color);
}

void UI::drawTimerBar() {

    int screenWidth = Window::GetWidth();
    int screenHeight = Window::GetHeight();

    int barThickness = 8;

    Draw::RectangleFilled({0, 0}, {(float)barThickness, (float)screenHeight}, BLACK); // Left
    Draw::RectangleFilled({(float)screenWidth - (float)barThickness, 0}, {(float)barThickness, (float)screenHeight}, BLACK); // Right
    Draw::RectangleFilled({0, 0}, {(float)screenWidth, (float)barThickness}, BLACK); // Top
    Draw::RectangleFilled({0, (float)screenHeight - (float)barThickness}, {(float)screenWidth, (float)barThickness}, BLACK); // Bottom


    rectf bars[5] = {
        {  (float)screenWidth / 2.f, 0, (float)screenWidth / 2.f, (float)barThickness }, // Top bar
        {  (float)screenWidth - (float)barThickness,  0.f, (float)barThickness,  (float)screenHeight }, // Right bar
        {  0.f,  (float)screenHeight - (float)barThickness,  (float)screenWidth, (float)barThickness }, // Bottom bar
        { 0.f,  0.f, (float)barThickness,  (float)screenHeight }, // Left bar
        { 0.f, 0.f,  (float)screenWidth / 2.f, (float)barThickness } // Top-left bar
    };

    double elapsed = Window::GetRunTime(); // Elapsed time in seconds
    double rate = 200.0; // Pixels per second

    float totalLength = bars[0].width + bars[1].height + bars[2].width + bars[3].height + bars[4].width;

    float totalBarLength = elapsed * rate; // na 1.0 seconde 100 pixels lang

    int rotations = std::floor(totalBarLength / totalLength);

    int barcolor = std::clamp(rotations, 0, 4);

    totalBarLength -= (float)rotations * totalLength;


    Color c[5] = {
            RED, GREEN, YELLOW, PURPLE, BLUE
    };

    if (bars[0].width > totalBarLength)
    {
        bars[0].width = totalBarLength;
    }

    if (bars[1].height > totalBarLength - bars[0].width)
    {
        bars[1].height = totalBarLength - bars[0].width;
    }

    if (bars[2].width > totalBarLength - (bars[0].width + bars[1].height))
    {
        bars[2].x = bars[2].width - (totalBarLength - (bars[0].width + bars[1].height));
    }

    if (bars[3].height > totalBarLength - (bars[0].width + bars[1].height + bars[2].width))
    {
        bars[3].y = bars[3].height - (totalBarLength - (bars[0].width + bars[1].height + bars[2].width));
    }

    if (bars[4].width > totalBarLength - (bars[0].width + bars[1].height + bars[2].width + bars[3].height))
    {
        bars[4].width = (totalBarLength - (bars[0].width + bars[1].height + bars[2].width + bars[3].height));
    }

    for (int i = 0; i < 5; ++i) {
        Draw::RectangleFilled({bars[i].x, bars[i].y}, {bars[i].width, bars[i].height}, c[barcolor]);
    }


}

[[maybe_unused]] void UI::DrawPlayerBar(flecs::entity entity) {
    const Texture uiElements = AssetHandler::GetTexture("assets/ui/ui.png");
    const Texture items = AssetHandler::GetTexture("assets/tilesets/items-1.png");

    rectf selectedPowerUpBox = {539, 460, 24, 24};

    auto player = entity.get_mut<PlayerInput>();
    auto* xp = entity.has<PlayerXP>() ? entity.get_mut<PlayerXP>() : nullptr;

    b2BodyId rigidBody2d = entity.get_mut<RigidBody2D>()->RigidBody;
    auto playerClass = entity.get<PlayerClass>();

    std::vector<int> weaponIcon = {0, 0, 0, 0};

    switch (playerClass->classtype) {
        case PlayerClassType::WIZARD:
            weaponIcon.at(0) = 64;
            weaponIcon.at(1) = 79;
            weaponIcon.at(2) = 72;
            weaponIcon.at(3) = 101;
            break;
        case PlayerClassType::WRONG_WIZARD:
            weaponIcon.at(0) = 63;
            weaponIcon.at(1) = 79;
            weaponIcon.at(2) = 72;
            weaponIcon.at(3) = 101;
            break;
        case PlayerClassType::PINBALL_WIZARD:
            weaponIcon.at(0) = 62;
            weaponIcon.at(1) = 79;
            weaponIcon.at(2) = 72;
            weaponIcon.at(3) = 101;
            break;
        case PlayerClassType::FROST_WIZARD:
            weaponIcon.at(0) = 61;
            weaponIcon.at(1) = 79;
            weaponIcon.at(2) = 72;
            weaponIcon.at(3) = 101;
            break;

    }

    int weaponCount = 4;
    float circleRadius = 80.0f;

    //WLLog::AddLine("Campos:", Helpers::TextFormat("X: %.2f Y: %.2f", Camera::GetTarget().x, Camera::GetTarget().y));

    b2Vec2 rb2dPos = b2Body_GetPosition(rigidBody2d);
    vf2d worldPos = Camera::ToScreenSpace(vf2d{rb2dPos.x, rb2dPos.y} * 32);

    float selectedWeaponAngleOffset = -((static_cast<float>(player->selectedWeapon - 1) / static_cast<float>(weaponCount)) * 2 * PI) + (PI / 2) * 3;

    //WLLog::AddLine("lerp:", Helpers::TextFormat("%.2f", player->weaponSelectLerp->getValue()));

    if (player->weaponSelectLerp->isFinished()) {
        if (xp && xp->isHolding) {
            // Keep wheel visible while upgrading
            player->weaponSelectVisibleLerp->started = false;
            player->weaponSelectVisibleLerp->time = 0.0f;
        } else {
            player->weaponSelectVisibleLerp->started = true;
        }
    }

    int previousWeapon = player->selectedWeapon - 1;

    if (player->moveWeaponLeft) {
        previousWeapon += 1;
    } else {
        previousWeapon -= 1;
    }

    if (previousWeapon < 0) previousWeapon = weaponCount - 1;
    if (previousWeapon > weaponCount - 1) previousWeapon = 0;


    for (int i = 0; i < weaponCount; ++i) {
        float angle = selectedWeaponAngleOffset + (static_cast<float>(i) / static_cast<float>(weaponCount)) * 2 * PI;

        if (!player->weaponSelectLerp->started) {
            angle -= (PI / 2);
        } else {
            if (player->moveWeaponLeft) {
                angle += (PI / 2) * 2 + (PI / 2) * player->weaponSelectLerp->getValue();
            } else {
                angle -= (PI / 2) * player->weaponSelectLerp->getValue();
            }

        }

        rectf itemPosition = {
                (worldPos.x + circleRadius * sin(angle)) - 8.f,
                (worldPos.y + circleRadius * cos(angle)) - 8.f,
                16.f,
                16.f,
        };

        rectf boxPosition = {
                itemPosition.x - 16,
                itemPosition.y - 18,
                48.f,
                48.f,
        };

        auto color = WHITE;

        if (player->selectedWeapon - 1 == i) {
            selectedPowerUpBox.y = 492;
            color.a = static_cast<unsigned char>(255.f * player->weaponSelectLerp->getValue());

            if (player->weaponSelectVisibleLerp->started) {
                color.a = static_cast<unsigned char>(255.f * (1.f - std::clamp(player->weaponSelectVisibleLerp->getValue(), 0.f, 1.f)));
            }

        } else if (previousWeapon == i) {
            if (player->weaponSelectLerp->started)
                color.a = static_cast<unsigned char>(255.f * (1.f - player->weaponSelectLerp->getValue()));
            else
                color.a = 0;

        } else {
            selectedPowerUpBox.y = 460;
            color.a = 0;
        }

        auto _getRectangle = [&](int x, int y) {
            return rectf{ static_cast<float>(x * Configuration::tileWidth), static_cast<float>(y * Configuration::tileHeight), static_cast<float>(Configuration::tileWidth), static_cast<float>(Configuration::tileHeight) };
        };

        auto _getTile = [&_getRectangle](int tileId) {
            return _getRectangle(tileId % 16, (int)tileId / 16);
        };


        Draw::TexturePart(uiElements, {boxPosition.x, boxPosition.y}, {boxPosition.width, boxPosition.height}, selectedPowerUpBox, color);
        auto weaponIconPos = _getTile(weaponIcon[i]);

        Draw::TexturePart(items, {itemPosition.x, itemPosition.y}, {itemPosition.width, itemPosition.height}, weaponIconPos, color);


    }

    // Upgrade progress ring and can't-afford indicator
    if (xp && xp->isHolding) {
        constexpr float ringRadius = 95.0f;
        constexpr int   ringSegments = 48;
        constexpr float ringThickness = 4.0f;
        const Color ringColor = { 255, 215, 0, 220 };

        float progress = xp->holdTimer / PlayerXP::HOLD_DURATION;
        int drawnSegments = static_cast<int>(progress * ringSegments);

        for (int s = 0; s < drawnSegments; ++s) {
            float a0 = (-PI / 2.f) + (static_cast<float>(s)       / ringSegments) * 2.f * PI;
            float a1 = (-PI / 2.f) + (static_cast<float>(s + 1)   / ringSegments) * 2.f * PI;
            vf2d p0 = { worldPos.x + ringRadius * cosf(a0), worldPos.y + ringRadius * sinf(a0) };
            vf2d p1 = { worldPos.x + ringRadius * cosf(a1), worldPos.y + ringRadius * sinf(a1) };
            Draw::ThickLine(p0, p1, ringColor, ringThickness);
        }
    }

    // Can't afford indicator — show when ACCEPT is held but XP is insufficient
    // TODO: replace sprite 5 from items-1.png with a proper "can't afford" icon
    if (xp) {
        int weaponIdx = player->selectedWeapon - 1;
        bool anyHeld = false;
        for (const auto& controller : player->controllers) {
            if (controller->is(Buttons::ACCEPT, Action::HELD)) { anyHeld = true; break; }
        }
        if (anyHeld && !xp->canAfford(player->weaponUpgrades[weaponIdx])) {
            rectf cantAffordSrc = { 5.f * static_cast<float>(Configuration::tileWidth), 0.f,
                                    static_cast<float>(Configuration::tileWidth), static_cast<float>(Configuration::tileHeight) };
            vf2d iconPos = { worldPos.x - 16.f, worldPos.y - 16.f };
            Draw::TexturePart(items, iconPos, { 32.f, 32.f }, cantAffordSrc, WHITE);
        }
    }
}

void UI::DrawMiniMap() {

    int miniMapSize = 200;

    auto data = World::getDungeon().getDungeonTileData();
    auto rooms = World::getDungeon().getRooms();

    int offsetLeft = (Window::GetWidth() / 2) - miniMapSize / 2;
    int offsetTop = Window::GetHeight() - miniMapSize;

    Draw::SetTargetRenderPass("minimap");


    auto borderColor = GRAY;

    for (const Room *room: rooms) {
        int miniMapX = room->pos.x * miniMapSize / data.width;
        int miniMapY = room->pos.y * miniMapSize / data.height;

        int miniMapWidth = room->size.x * miniMapSize / data.width;
        int miniMapHeight = room->size.y * miniMapSize / data.height;

        auto bgColor = BLACK;

        if (room->start) {
            bgColor = DARKGREEN;
        } else if (room->end) {
            bgColor = {115, 21, 27, 255};
        }

        if (room->specialRoom) {
            bgColor = PURPLE;
        }

        Draw::RectangleFilled({(float)miniMapX, (float)miniMapY + 1.f},
                                {(float)miniMapWidth, (float)miniMapHeight},
                      bgColor);
    }

    // Draw borders with actual gaps where doors are, so z-ordering never hides them.
    // Each side is drawn as 1px-thick filled segments that skip door positions.
    for (const Room *room: rooms) {
        // Compute edges using the room's far corner rather than pos+size independently,
        // so adjacent rooms share exactly the same pixel boundary (avoids integer-division gaps).
        int miniMapX  = room->pos.x * miniMapSize / data.width;
        int miniMapY  = room->pos.y * miniMapSize / data.height;
        int miniMapX2 = (room->pos.x + room->size.x) * miniMapSize / data.width;
        int miniMapY2 = (room->pos.y + room->size.y) * miniMapSize / data.height;
        int miniMapWidth  = miniMapX2 - miniMapX;
        int miniMapHeight = miniMapY2 - miniMapY;
        int roomSizeX = room->size.x / 34;
        int roomSizeY = room->size.y / 34;
        int cellPxX   = (roomSizeX > 0) ? miniMapWidth  / roomSizeX : miniMapWidth;
        int cellPxY   = (roomSizeY > 0) ? miniMapHeight / roomSizeY : miniMapHeight;
        int doorGap   = 4;

        // Horizontal border side: draw segments along y, skipping door cells.
        // Gap is clamped to the wall's extent so it never overruns the room boundary.
        auto drawHSide = [&](int y, int exits, int numCells, int cellPx) {
            int wallEnd = miniMapX + miniMapWidth;
            int segX = miniMapX;
            for (int i = 0; i < numCells; i++) {
                if (exits & (1 << i)) {
                    int gapX    = std::max(segX, miniMapX + i * cellPx + cellPx / 2 - doorGap / 2);
                    int gapEnd  = std::min(gapX + doorGap, wallEnd);
                    if (gapX > segX)
                        Draw::RectangleFilled({(float)segX, (float)y}, {(float)(gapX - segX), 1.f}, borderColor);
                    segX = gapEnd;
                }
            }
            if (segX <= wallEnd)
                Draw::RectangleFilled({(float)segX, (float)y}, {(float)(wallEnd + 1 - segX), 1.f}, borderColor);
        };

        // Vertical border side: draw segments along x, skipping door cells.
        auto drawVSide = [&](int x, int exits, int numCells, int cellPx) {
            int wallEnd = miniMapY + miniMapHeight;
            int segY = miniMapY;
            for (int i = 0; i < numCells; i++) {
                if (exits & (1 << i)) {
                    int gapY   = std::max(segY, miniMapY + i * cellPx + cellPx / 2 - doorGap / 2);
                    int gapEnd = std::min(gapY + doorGap, wallEnd);
                    if (gapY > segY)
                        Draw::RectangleFilled({(float)x, (float)segY}, {1.f, (float)(gapY - segY)}, borderColor);
                    segY = gapEnd;
                }
            }
            if (segY <= wallEnd)
                Draw::RectangleFilled({(float)x, (float)segY}, {1.f, (float)(wallEnd + 1 - segY)}, borderColor);
        };

        drawHSide(miniMapY,  room->northExits, roomSizeX, cellPxX);
        drawHSide(miniMapY2, room->southExits, roomSizeX, cellPxX);
        drawVSide(miniMapX,  room->westExits,  roomSizeY, cellPxY);
        drawVSide(miniMapX2, room->eastExits,  roomSizeY, cellPxY);
    }

    const auto playerFilter = ECS::getWorld().filter<PlayerIndex>();
    playerFilter.each([&](flecs::entity entity, PlayerIndex index) {
        b2BodyId rigidBody2D = entity.get_mut<RigidBody2D>()->RigidBody;

        b2Vec2 b2pos = b2Body_GetPosition(rigidBody2D);
        vf2d pos = {b2pos.x, b2pos.y};

        pos.x *= static_cast<float>(miniMapSize) / static_cast<float>(data.width);
        pos.y *= static_cast<float>(miniMapSize) / static_cast<float>(data.height);

        auto playerColor = WHITE;

        playerColor.a = static_cast<unsigned char>(255.f - (std::fmod(Window::GetRunTime(), 0.4f) * 600.f));

        Draw::CircleFilled(pos, 2.f, playerColor);


    });


    Draw::ResetTargetRenderPass();

    auto *fb = Renderer::GetFramebuffer("minimap_framebuffer");
    Draw::Texture(fb->textureView, {static_cast<float>(offsetLeft), static_cast<float>(offsetTop + 1)}, {static_cast<float>(miniMapSize), static_cast<float>(miniMapSize)}, {255, 255, 255, 200});

}

[[maybe_unused]] void UI::drawDebugInfo() {
    constexpr int baseX = 30;
    constexpr int baseY = 270;
    int margin = 20;

    AssetHandler::GetFont("assets/fonts/APL386.ttf", 20);

    int y = 0;


    WLLog::AddLine("FPS:", Helpers::TextFormat("%d", Window::GetFPS()));

    WLLog::AddLine("GameTime:", Helpers::TextFormat("%.2f", Configuration::gameTime));
    WLLog::AddLine("Bodies awake:", Helpers::TextFormat("%d", b2World_GetBodyEvents(World::getPhysicsWorld()).moveCount));
    WLLog::AddLine("SlowMoF:", Helpers::TextFormat("%f", Configuration::slowMotionFactor));


    WLLog::AddLine("");
    WLLog::AddLine("-- Players --");

    const auto playerFilter = ECS::getWorld().filter<PlayerIndex, RigidBody2D>();
    playerFilter.each([&](flecs::entity entity, PlayerIndex playerIndex, RigidBody2D rigidBody2D) {
        WLLog::AddLine(
                Helpers::TextFormat("Player %d pos:", playerIndex.index),
                Helpers::TextFormat("X: %.2f Y: %.2f", b2Body_GetPosition(rigidBody2D.RigidBody).x,
                           b2Body_GetPosition(rigidBody2D.RigidBody).y)
        );
    });

    auto lines = WLLog::getLines();

    auto debugFont = AssetHandler::GetFont("assets/fonts/APL386.ttf", 18);
    for (const auto &line : lines) {
        WLLog::setHeaderWidth(Text::MeasureText(debugFont, line.first, 18));
        WLLog::setLongestLineWidth(Text::MeasureText(debugFont, line.second, 18));
    }

    rectf backgroundRect = {
            static_cast<float>(baseX - margin),
            static_cast<float>(baseY - margin),
            static_cast<float>(WLLog::getHeaderWidth() + WLLog::getLongestLineWidth() + (margin * 3)),
            static_cast<float>((lines.size() * 25) + (margin * 2) - 10)
    };

    Draw::RectangleRoundedFilled({backgroundRect.x, backgroundRect.y}, {backgroundRect.width, backgroundRect.height}, 10.f, {0, 0, 0, 127});


    for (const auto &line: lines) {
        Text::DrawText(debugFont, {(float)baseX, (float)(baseY + y)}, line.first.c_str(), LIME);
        Text::DrawText(debugFont, {(float)(baseX + WLLog::getHeaderWidth() + margin), (float)(baseY + y)}, line.second.c_str(), LIME);

        y += 25;
    }


}
