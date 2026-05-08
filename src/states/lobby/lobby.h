#pragma once
#include <luminoveau.h>
#include <state/basestate.h>
#include "components/playerclass.h"

class LobbyState : public BaseState {
public:
    void load() override;
    void unload() override;
    void draw() override;

private:
    enum class SlotState { Empty, Selecting, Ready };

    struct Slot {
        SlotState state = SlotState::Empty;
        int controllerIndex = -1;
        PlayerClassType classType = PlayerClassType::WIZARD;
    };

    Slot slots[4];

    static constexpr int CLASS_COUNT = 4;
    static constexpr const char* CLASS_NAMES[CLASS_COUNT] = { "WIZARD", "WRONG WIZARD", "PINBALL WIZARD", "FROST WIZARD" };
    static constexpr Color CLASS_COLORS[CLASS_COUNT] = {
        { 80,  80,  210, 255 },  // Wizard        — blue
        { 210, 60,  60,  255 },  // Wrong Wizard  — red
        { 210, 160, 20,  255 },  // Pinball Wizard — gold
        { 60,  200, 220, 255 },  // Frost Wizard  — ice blue
    };

    void cycleClass(int slotIndex, int dir);
    bool allReadyOrEmpty() const;
    void startGame();

    void drawSlot(int slotIndex, int x, int y, int w, int h);

    TextureAsset entitySprite;
    float animTimer = 0.f;

    ShaderAsset passthroughVert;
    ShaderAsset hueShiftFrag;
    // One effect per player slot — slot 0 has no shift
    EffectAsset slotEffects[4];
};
