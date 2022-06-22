#pragma once

#include "server/gamemode/GameMode.hpp"
#include "server/gamemode/GameModeInfoBase.hpp"
#include "server/gamemode/GameModeTimer.hpp"

struct SardineInfo : GameModeInfoBase {
    SardineInfo() {
        mMode = GameMode::SARDINE;
    }
    bool mIsIt            = false;
    bool mIsUseGravityCam = false;

    static bool mIsUseGravity;
    static bool mIsTether;
    static bool mIsTetherSnap;

    GameTime mHidingTime;
};
