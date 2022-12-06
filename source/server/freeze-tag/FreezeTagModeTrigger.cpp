#include "server/freeze-tag/FreezeTagMode.hpp"
#include "al/util.hpp"
#include "al/util/LiveActorUtil.h"
#include "al/util/RandomUtil.h"
#include "math/seadVector.h"
#include "puppets/PuppetInfo.h"
#include "server/gamemode/GameModeManager.hpp"
#include "server/freeze-tag/FreezeTagIcon.h"
#include "al/alCollisionUtil.h"

void FreezeTagMode::startRound(int roundMinutes) {
    mInfo->mIsRound = true;
    mInfo->mFreezeCount = 0;

    mInvulnTime = 0.f;

    mModeTimer->enableTimer();
    mModeTimer->disableControl();
    mModeTimer->setTimerDirection(false);

    // Start timer at roundMinutes - 1s (to not instantly set off a score event that happens every full minute)
    mModeTimer->setTime(0.f, 59, roundMinutes - 1, 0);

    // remember who is part of this round
    for (int i = 0; i < mInfo->mRunnerPlayers.size(); i++) {
        mInfo->mRunnerPlayers.at(i)->isFreezeInRound = true;
    }
    for (int i = 0; i < mInfo->mChaserPlayers.size(); i++) {
        mInfo->mChaserPlayers.at(i)->isFreezeInRound = true;
    }
}

void FreezeTagMode::endRound(bool isAbort) {
    mInfo->mIsRound = false;
    mInfo->mFreezeCount = 0;

    mModeTimer->disableTimer();

    if (!mIsEndgameActive) {
        // transform: chaser => runner
        if (!mInfo->mIsPlayerRunner) {
            mInfo->mIsPlayerRunner = true;
            sendFreezePacket(FreezeUpdateType::PLAYER);
            return;
        }

        // get points for winning
        if (!isAbort) {
            mInfo->mPlayerTagScore.eventScoreRunnerWin();
        }

        // unfreeze
        if (mInfo->mIsPlayerFreeze) {
            trySetPlayerRunnerState(FreezeState::ALIVE);
        }
    }

    // forget who was part of this round
    for (int i = 0; i < mPuppetHolder->getSize(); i++) {
        Client::getPuppetInfo(i)->isFreezeInRound = false;
    }
}

/*
 * SET THE RUNNER PLAYER'S FROZEN/ALIVE STATE
 */
bool FreezeTagMode::trySetPlayerRunnerState(FreezeState newState) {
    if (mInfo->mIsPlayerFreeze == newState || !mInfo->mIsPlayerRunner) {
        return false;
    }

    PlayerActorHakoniwa* player = getPlayerActorHakoniwa();
    if (!player) {
        return false;
    }

    mInvulnTime = 0.f;

    if (newState == FreezeState::ALIVE) {
        mInfo->mIsPlayerFreeze = FreezeState::ALIVE;

        player->endDemoPuppetable();

        sendFreezePacket(FreezeUpdateType::PLAYER);
    } else if (!mInfo->mIsRound) {
        return false;
    } else {
        mInfo->mIsPlayerFreeze = FreezeState::FREEZE;

        if (player->getPlayerHackKeeper()->currentHackActor) { // we're in a capture
            player->getPlayerHackKeeper()->cancelHackArea(); // leave the capture
        }

        player->startDemoPuppetable();
        player->mPlayerAnimator->endSubAnim();
        player->mPlayerAnimator->startAnim("DeadIce");
        player->mHackCap->forcePutOn();

        mSpectateIndex = -1;
        mInfo->mFreezeCount++;

        sendFreezePacket(FreezeUpdateType::PLAYER);

        if (areAllOtherRunnersFrozen(nullptr)) {
            // if there is only one runner, then end the round for legacy clients (new clients do this themselves)
            if (1 == mInfo->mRunnerPlayers.size() + mInfo->mIsPlayerRunner) {
                mCancelOnlyLegacy = true;
                sendFreezePacket(FreezeUpdateType::ROUNDCANCEL);
            }

            tryStartEndgameEvent();
        }
    }

    return true;
}

/*
 * START AN ENDGAME EVENT (wipeout)
 */
void FreezeTagMode::tryStartEndgameEvent() {
    mIsEndgameActive = true;
    mEndgameTimer    = 0.f;
    mModeLayout->showEndgameScreen();

    PlayerActorHakoniwa* player = getPlayerActorHakoniwa();
    if (!player) {
        return;
    }

    if (player->getPlayerHackKeeper()->currentHackActor) { // we're in a capture
        player->getPlayerHackKeeper()->cancelHackArea(); // leave the capture
    }

    player->startDemoPuppetable();
    rs::faceToCamera(player);
    player->mPlayerAnimator->endSubAnim();

    if (mInfo->mIsPlayerRunner) {
        player->mPlayerAnimator->startAnim("RaceResultLose");
        trySetPostProcessingType(FreezePostProcessingType::PPENDGAMELOSE);
    } else {
        player->mPlayerAnimator->startAnim("RaceResultWin");
        trySetPostProcessingType(FreezePostProcessingType::PPENDGAMEWIN);
        mInfo->mPlayerTagScore.eventScoreWipeout();
    }

    endRound(false);
}

/*
 * HANDLE PLAYER RECOVERY
 * Player recovery is started by entering a death area or an endgame (wipeout)
 */
bool FreezeTagMode::tryStartRecoveryEvent(bool isEndgame) {
    if (mRecoveryEventFrames > 0 || !mWipeHolder) {
        return false; // Something isn't applicable here, return fail
    }

    PlayerActorHakoniwa* player = getPlayerActorHakoniwa();
    if (!player) {
        return false;
    }

    mRecoveryEventFrames = (mRecoveryEventLength / 2) * (isEndgame + 1);
    mWipeHolder->startClose("FadeBlack", (mRecoveryEventLength / 4) * (isEndgame + 1));

    if (!isEndgame) {
        mRecoverySafetyPoint = player->mPlayerRecoverySafetyPoint->mSafetyPointPos;
        if (mInfo->mIsPlayerRunner && mInfo->mIsRound) {
            sendFreezePacket(FreezeUpdateType::FALLOFF);
        }
    } else {
        mRecoverySafetyPoint = sead::Vector3f::zero;
    }

    Logger::log("Recovery event %.00fx %.00fy %.00fz\n", mRecoverySafetyPoint.x, mRecoverySafetyPoint.y, mRecoverySafetyPoint.z);

    return true;
}

bool FreezeTagMode::tryEndRecoveryEvent() {
    if (!mWipeHolder) {
        return false; //Recovery event is already started, return fail
    }

    mWipeHolder->startOpen(mRecoveryEventLength / 2);

    PlayerActorHakoniwa* player = getPlayerActorHakoniwa();
    if (!player) {
        return false;
    }

    // Set the player to frozen if they are a runner AND they had a valid recovery point
    if (mInfo->mIsPlayerRunner && mRecoverySafetyPoint != sead::Vector3f::zero && mInfo->mIsRound) {
        trySetPlayerRunnerState(FreezeState::FREEZE);
        warpToRecoveryPoint(player);
    } else {
        trySetPlayerRunnerState(FreezeState::ALIVE);
        trySetPostProcessingType(FreezePostProcessingType::PPDISABLED);
    }

    // If player is a chaser with a valid recovery point, teleport (and disable collisions)
    if (!mInfo->mIsPlayerRunner || !mInfo->mIsRound) {
        player->startDemoPuppetable();
        if (mRecoverySafetyPoint != sead::Vector3f::zero) {
            warpToRecoveryPoint(player);
        }

        trySetPostProcessingType(FreezePostProcessingType::PPDISABLED);
    }

    // If player is being made alive, force end demo puppet state
    if (!mInfo->mIsPlayerFreeze) {
        player->endDemoPuppetable();
    }

    if (!mIsEndgameActive) {
        mModeLayout->hideEndgameScreen();
    }

    return true;
}

/*
 * UPDATE PLAYER SCORES
 * FUNCTION CALLED FROM FreezeTagMode.cpp ON RECEIVING FREEZE TAG PACKETS
 */
void FreezeTagMode::tryScoreEvent(FreezeTagPacket* packet, PuppetInfo* other) {
    if (!mCurScene || !mCurScene->mIsAlive || !GameModeManager::instance()->isModeAndActive(GameMode::FREEZETAG)) {
        return;
    }

    // Only if the other player is a runner
    if (!other || !other->isFreezeTagRunner) {
        return;
    }

    // Only if the frozen state of the other player changes
    if (other->isFreezeTagFreeze == packet->isFreeze) {
        return;
    }

    // Check if we unfreeze a fellow runner
    bool scoreUnfreeze = (
            mInfo->mIsPlayerRunner      // we are a runner
        && !mInfo->mIsPlayerFreeze      // that is unfrozen and are touching another runner
        && !other->isFreezeTagFallenOff // that was not frozen by falling off the map
        && !packet->isFreeze            // which is unfreezing right now
    );

    // Check if we freeze a runner as a chaser
    bool scoreFreeze = (
           !scoreUnfreeze
        && !mInfo->mIsPlayerRunner // we are a chaser touching a runner
        && packet->isFreeze        // which is freezing up right now
    );

    if (other->isInSameStage && (scoreUnfreeze || scoreFreeze)) {
        PlayerActorBase* playerBase = rs::getPlayerActor(mCurScene);

        // Calculate the distance to the other player
        float distanceSq = playerBase ? vecDistanceSq(al::getTrans(playerBase), other->playerPos) : 999999.f;

        // Only apply the score event if the runner is less than 600 units away
        if (distanceSq < 360000.f) { // non-squared: 600.0
            if (scoreUnfreeze) {
                mInfo->mPlayerTagScore.eventScoreUnfreeze();
            } else {
                mInfo->mPlayerTagScore.eventScoreFreeze();
            }
        }
    }

    // Checks if every runner is frozen, starts endgame sequence if so
    if (packet->isFreeze && areAllOtherRunnersFrozen(other)) {
        // if there is only one runner, then end the round for legacy clients (new clients do this themselves)
        if (1 == mInfo->mRunnerPlayers.size() + mInfo->mIsPlayerRunner) {
            mCancelOnlyLegacy = true;
            sendFreezePacket(FreezeUpdateType::ROUNDCANCEL);
        }

        tryStartEndgameEvent();
    }
}

/*
 * SET THE POST PROCESSING STYLE IN THE GAMEMODE
 */
bool FreezeTagMode::trySetPostProcessingType(FreezePostProcessingType type) {
    if (!mCurScene) { // doesn't have a scene
      return false;
    }

    u8  ppIdx  = type;
    u32 curIdx = al::getPostProcessingFilterPresetId(mCurScene);
    if (ppIdx == curIdx) {
        return false; // Already set to target post processing type => return fail
    }

    // loop trough the filters till we reach the desired one
    while (curIdx != ppIdx) {
        al::incrementPostProcessingFilterPreset(mCurScene);
        curIdx = (curIdx + 1) % 18;
    }

    if (type == FreezePostProcessingType::PPDISABLED) {
        al::invalidatePostProcessingFilter(mCurScene);
        return true; // Disabled current post processing mode
    }

    al::validatePostProcessingFilter(mCurScene);

    Logger::log("Set post processing to %i\n", al::getPostProcessingFilterPresetId(mCurScene));

    return true; // Set post processing mode to on at desired index
}

void FreezeTagMode::warpToRecoveryPoint(al::LiveActor* actor) {
    // warp to a random chaser if we are a runner during a round
    if (mInfo->mIsRound && mInfo->mIsPlayerRunner && 0 < mInfo->mChaserPlayers.size()) {
        int size = mInfo->mChaserPlayers.size();
        int rnd  = al::getRandom(size);
        int i    = rnd;

        do {
            // to be suitable the chaser needs to be connected and in the same stage
            PuppetInfo* chaser = mInfo->mChaserPlayers.at(i);
            if (chaser && chaser->isConnected && !chaser->isFreezeTagRunner && chaser->isInSameStage) {
                al::setTrans(actor, chaser->playerPos);
                return;
            }
            // check the next chaser, if the randomly chosen one is unsuitable
            i = (i + 1) % size; // loop around
        } while (i != rnd); // unless all chasers are unsuitable
    }

    // warp to the last safe recovery point
    al::setTrans(actor, mRecoverySafetyPoint);
}
