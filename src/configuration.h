#pragma once

namespace Configuration {
    // Tile dimensions
    inline constexpr int tileWidth  = 32;
    inline constexpr int tileHeight = 32;

    // Runtime state
    inline float gameTime         = 0.0f;
    inline float slowMotionFactor = 1.0f;
    inline bool  showFPS          = false;
    inline bool  showGameStats    = false;
}
