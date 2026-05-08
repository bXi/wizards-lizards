#pragma once

struct PlayerXP {
    float xp = 0.f;

    float holdTimer = 0.f;
    bool isHolding = false;

    static constexpr float HOLD_DURATION = 1.0f;

    [[nodiscard]] float upgradeCost(int currentUpgrades) const {
        return static_cast<float>((currentUpgrades + 1) * 10);
    }

    [[nodiscard]] bool canAfford(int currentUpgrades) const {
        return xp >= upgradeCost(currentUpgrades);
    }

    bool spend(int currentUpgrades) {
        float cost = upgradeCost(currentUpgrades);
        if (xp < cost) return false;
        xp -= cost;
        return true;
    }
};
