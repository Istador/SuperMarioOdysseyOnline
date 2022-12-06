#pragma once

#include "puppets/PuppetInfo.h"
#include "server/gamemode/GameModeInfoBase.hpp"
#include "server/gamemode/GameModeTimer.hpp"
#include "server/freeze-tag/FreezeTagScore.hpp"

enum FreezeState { // Runner team player's state
    ALIVE  = 0,
    FREEZE = 1,
};

struct FreezeTagInfo : GameModeInfoBase {
    FreezeTagInfo() { mMode = GameMode::FREEZETAG; }

    bool        mIsPlayerRunner = true;
    float       mFreezeIconSize = 0.f;
    FreezeState mIsPlayerFreeze = FreezeState::ALIVE;

    bool     mIsRound = false;
    int      mFreezeCount = 0;
    GameTime mRoundTimer;

    sead::PtrArray<PuppetInfo> mRunnerPlayers;
    sead::PtrArray<PuppetInfo> mChaserPlayers;
    sead::PtrArray<PuppetInfo> mOtherPlayers;

    static FreezeTagScore mPlayerTagScore;
    static int            mRoundLength; // Length of rounds in minutes
    static bool           mIsHostMode;
    static bool           mIsDebugMode;
};
