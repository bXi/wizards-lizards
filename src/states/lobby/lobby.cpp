#include "lobby.h"
#include "lobbyconfig.h"
#include <state/state.h>

void LobbyState::load() {
    slots[0].state = SlotState::Selecting;
    slots[0].controllerIndex = 0;
    slots[1] = {}; slots[2] = {}; slots[3] = {};

    entitySprite  = AssetHandler::GetTexture("assets/entities/entities.png");
    passthroughVert = AssetHandler::GetShader("assets/shaders/effect_passthrough.vert");
    hueShiftFrag    = AssetHandler::GetShader("assets/shaders/effect_hueshift.frag");

    constexpr float hueShifts[4] = { 0.0f, 0.333f, 0.667f, 0.5f };
    for (int i = 0; i < 4; ++i) {
        slotEffects[i] = Effects::Create(passthroughVert, hueShiftFrag);
        slotEffects[i]["hueShift"] = hueShifts[i];
    }
}

void LobbyState::unload() {}

void LobbyState::cycleClass(int slotIndex, int dir) {
    int c = static_cast<int>(slots[slotIndex].classType);
    c = (c + dir + CLASS_COUNT) % CLASS_COUNT;
    slots[slotIndex].classType = static_cast<PlayerClassType>(c);
}

bool LobbyState::allReadyOrEmpty() const {
    for (int i = 0; i < 4; ++i) {
        if (slots[i].state == SlotState::Selecting) return false;
    }
    return true;
}

void LobbyState::startGame() {
    LobbyConfig& cfg = LobbyConfig::get();
    cfg.reset();
    for (int i = 0; i < 4; ++i) {
        if (slots[i].state != SlotState::Empty) {
            cfg.players[i].active = true;
            cfg.players[i].classType = slots[i].classType;
            cfg.players[i].controllerIndex = slots[i].controllerIndex;
        }
    }
    State::SetState("game");
}

void LobbyState::draw() {
    const int sw = Window::GetWidth();
    const int sh = Window::GetHeight();
    const int slotW = sw / 4;

    auto inputs = Input::GetAllInputs();

    // Build set of already-assigned controller indices
    bool assigned[16] = {};
    for (int i = 0; i < 4; ++i)
        if (slots[i].controllerIndex >= 0) assigned[slots[i].controllerIndex] = true;

    for (int i = 0; i < 4; ++i) {
        Slot& slot = slots[i];
        InputDevice* dev = (slot.controllerIndex >= 0 && slot.controllerIndex < (int)inputs.size())
                           ? inputs[slot.controllerIndex] : nullptr;

        if (slot.state == SlotState::Selecting && dev) {
            if (dev->is(Buttons::LEFT,  Action::PRESSED)) cycleClass(i, -1);
            if (dev->is(Buttons::RIGHT, Action::PRESSED)) cycleClass(i,  1);
            if (dev->is(Buttons::ACCEPT, Action::PRESSED)) slot.state = SlotState::Ready;
            if (i > 0 && dev->is(Buttons::BACK, Action::PRESSED)) {
                slot.state = SlotState::Empty;
                assigned[slot.controllerIndex] = false;
                slot.controllerIndex = -1;
            }
        } else if (slot.state == SlotState::Ready && dev) {
            if (dev->is(Buttons::BACK, Action::PRESSED)) slot.state = SlotState::Selecting;
            // P1 starts the game
            if (i == 0 && allReadyOrEmpty() && dev->is(Buttons::ACCEPT, Action::PRESSED)) {
                startGame();
                return;
            }
        }
    }

    // Any unassigned device pressing ACCEPT joins the next empty slot
    for (int ci = 0; ci < (int)inputs.size(); ++ci) {
        if (assigned[ci]) continue;
        if (inputs[ci]->is(Buttons::ACCEPT, Action::PRESSED)) {
            for (int i = 1; i < 4; ++i) {
                if (slots[i].state == SlotState::Empty) {
                    slots[i].state = SlotState::Selecting;
                    slots[i].controllerIndex = ci;
                    slots[i].classType = PlayerClassType::WIZARD; // new players start on Wizard
                    assigned[ci] = true;
                    break;
                }
            }
        }
    }

    // Back from lobby to menu (P1 presses BACK while Selecting)
    if (slots[0].state == SlotState::Selecting) {
        InputDevice* p1dev = inputs.size() > 0 ? inputs[0] : nullptr;
        if (p1dev && p1dev->is(Buttons::BACK, Action::PRESSED)) {
            State::SetState("menu");
            return;
        }
    }

    // --- Draw ---
    auto fontMedium = AssetHandler::GetFont("assets/fonts/APL386.ttf", 20);

    animTimer += Window::GetFrameTime();

    for (int i = 0; i < 4; ++i) {
        drawSlot(i, i * slotW, 0, slotW, sh);
    }

    // Bottom hint
    bool canStart = slots[0].state == SlotState::Ready && allReadyOrEmpty();
    const char* hint = canStart ? "P1: PRESS ACCEPT TO START" : "Waiting for players to ready up...";
    Color hintColor = canStart ? Color{255, 215, 0, 255} : Color{180, 180, 180, 255};
    int hintW = Text::MeasureText(fontMedium, hint, 20);
    Text::DrawText(fontMedium, { (float)(sw / 2 - hintW / 2), (float)(sh - 50) }, hint, hintColor);
}

void LobbyState::drawSlot(int i, int x, int y, int w, int h) {
    const Slot& slot = slots[i];

    auto fontLarge  = AssetHandler::GetFont("assets/fonts/APL386.ttf", 40);
    auto fontMedium = AssetHandler::GetFont("assets/fonts/APL386.ttf", 20);
    auto fontSmall  = AssetHandler::GetFont("assets/fonts/APL386.ttf", 16);

    const int pad = 12;

    // Background
    Color bg = { 30, 30, 30, 255 };
    if (slot.state != SlotState::Empty) {
        Color classCol = CLASS_COLORS[static_cast<int>(slot.classType)];
        bg = { (uint8_t)(classCol.r / 4), (uint8_t)(classCol.g / 4), (uint8_t)(classCol.b / 4), 255 };
    }
    Draw::RectangleFilled({ (float)(x + 2), (float)(y + 2) }, { (float)(w - 4), (float)(h - 4) }, bg);

    // Border — brighter when ready
    Color border = slot.state == SlotState::Ready ? Color{255, 215, 0, 255} : Color{80, 80, 80, 255};
    Draw::Rectangle({ (float)(x + 2), (float)(y + 2) }, { (float)(w - 4), (float)(h - 4) }, border);

    // "PLAYER X"
    const char* playerLabel = Helpers::TextFormat("PLAYER %d", i + 1);
    int labelW = Text::MeasureText(fontMedium, playerLabel, 20);
    Text::DrawText(fontMedium, { (float)(x + w / 2 - labelW / 2), (float)(y + 40) }, playerLabel, WHITE);

    if (slot.state == SlotState::Empty) {
        const char* joinText = i == 0 ? "PRESS ACCEPT" : "PRESS ACCEPT TO JOIN";
        int jw = Text::MeasureText(fontSmall, joinText, 16);
        Text::DrawText(fontSmall, { (float)(x + w / 2 - jw / 2), (float)(y + h / 2) }, joinText, { 120, 120, 120, 255 });
        return;
    }

    // Animated sprite preview
    {
        constexpr int SPRITE_INDEX = 16; // wizard row — shared by all classes for now
        constexpr int FRAME_COUNT  = 7;
        constexpr float FPS        = 8.f;
        constexpr int TILE         = 32; // px per tile in sheet

        int frame    = static_cast<int>(animTimer * FPS) % FRAME_COUNT;
        int tileId   = SPRITE_INDEX + frame;
        int col      = tileId % 16;
        int row      = tileId / 16;

        // doubleHeight: source is 32×64, starting one tile above the tile row
        rectf src = {
            static_cast<float>(col * TILE),
            static_cast<float>(row * TILE - TILE), // one row up for the head
            static_cast<float>(TILE),
            static_cast<float>(TILE * 2)
        };

        constexpr float SCALE = 4.f;
        float dw = TILE * SCALE;
        float dh = TILE * 2 * SCALE;
        float dx = x + w / 2.f - dw / 2.f;
        float dy = y + h * 0.12f;

        // Count earlier slots with the same class to determine shift index
        int shiftIndex = 0;
        for (int j = 0; j < i; ++j)
            if (slots[j].state != SlotState::Empty && slots[j].classType == slot.classType)
                shiftIndex++;

        if (shiftIndex > 0) Draw::SetEffect(slotEffects[shiftIndex]);
        Draw::TexturePart(entitySprite, { dx, dy }, { dw, dh }, src);
        if (shiftIndex > 0) Draw::ClearEffects();
    }

    // Class name
    const char* className = CLASS_NAMES[static_cast<int>(slot.classType)];
    Color classColor = CLASS_COLORS[static_cast<int>(slot.classType)];
    int cnW = Text::MeasureText(fontLarge, className, 40);
    Text::DrawText(fontLarge, { (float)(x + w / 2 - cnW / 2), (float)(y + h * 0.62f) }, className, classColor);

    if (slot.state == SlotState::Selecting) {
        // Navigation arrows
        Text::DrawText(fontMedium, { (float)(x + pad + 4), (float)(y + h / 2 - 30) }, "<", { 200, 200, 200, 255 });
        Text::DrawText(fontMedium, { (float)(x + w - pad - 20), (float)(y + h / 2 - 30) }, ">", { 200, 200, 200, 255 });

        const char* hint = "ACCEPT to ready";
        int hw = Text::MeasureText(fontSmall, hint, 16);
        Text::DrawText(fontSmall, { (float)(x + w / 2 - hw / 2), (float)(y + h - 100) }, hint, { 150, 150, 150, 255 });
    } else if (slot.state == SlotState::Ready) {
        const char* ready = "READY!";
        int rw = Text::MeasureText(fontMedium, ready, 20);
        Text::DrawText(fontMedium, { (float)(x + w / 2 - rw / 2), (float)(y + h - 100) }, ready, { 80, 220, 80, 255 });
    }
}
