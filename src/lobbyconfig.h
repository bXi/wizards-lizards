#pragma once
#include "components/playerclass.h"

struct LobbyConfig {
    struct PlayerSlot {
        bool active = false;
        PlayerClassType classType = PlayerClassType::WIZARD;
        int controllerIndex = -1;
    };

    PlayerSlot players[4];

    int activeCount() const {
        int n = 0;
        for (const auto& p : players) if (p.active) n++;
        return n;
    }

    void reset() {
        for (auto& p : players) p = {};
    }

    static LobbyConfig& get() { static LobbyConfig instance; return instance; }
};
