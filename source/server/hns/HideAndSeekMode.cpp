#include "server/hns/HideAndSeekMode.hpp"
#include <cmath>
#include "al/async/FunctorV0M.hpp"
#include "al/util.hpp"
#include "al/util/ControllerUtil.h"
#include "game/GameData/GameDataHolderAccessor.h"
#include "game/Layouts/CoinCounter.h"
#include "game/Layouts/MapMini.h"
#include "game/Player/PlayerActorBase.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "heap/seadHeapMgr.h"
#include "layouts/HideAndSeekIcon.h"
#include "logger.hpp"
#include "rs/util.hpp"
#include "server/gamemode/GameModeBase.hpp"
#include "server/Client.hpp"
#include "server/gamemode/GameModeTimer.hpp"
#include <heap/seadHeap.h>
#include "server/gamemode/GameModeManager.hpp"
#include "server/gamemode/GameModeFactory.hpp"

#include "basis/seadNew.h"
#include "server/hns/HideAndSeekConfigMenu.hpp"

HideAndSeekMode::HideAndSeekMode(const char* name) : GameModeBase(name) {}

void HideAndSeekMode::init(const GameModeInitInfo& info) {
    mSceneObjHolder = info.mSceneObjHolder;
    mMode           = info.mMode;
    mCurScene       = (StageScene*)info.mScene;
    mPuppetHolder   = info.mPuppetHolder;

    GameModeInfoBase* curGameInfo = GameModeManager::instance()->getInfo<HideAndSeekInfo>();

    if (curGameInfo) {
        Logger::log("Gamemode info found: %s %s\n", GameModeFactory::getModeString(curGameInfo->mMode), GameModeFactory::getModeString(info.mMode));
    } else {
        Logger::log("No gamemode info found\n");
    }

    if (curGameInfo && curGameInfo->mMode == mMode) {
        mInfo = (HideAndSeekInfo*)curGameInfo;
        mModeTimer = new GameModeTimer(mInfo->mHidingTime);
        Logger::log("Reinitialized timer with time %d:%.2d\n", mInfo->mHidingTime.mMinutes, mInfo->mHidingTime.mSeconds);
    } else {
        if (curGameInfo) {
            delete curGameInfo; // attempt to destory previous info before creating new one
        }

        mInfo = GameModeManager::instance()->createModeInfo<HideAndSeekInfo>();

        mModeTimer = new GameModeTimer();
    }

    mModeLayout = new HideAndSeekIcon("HideAndSeekIcon", *info.mLayoutInitInfo);

    mModeLayout->showSeeking();

    mModeTimer->disableTimer();
}

void HideAndSeekMode::begin() {
    mModeLayout->appear();

    mIsFirstFrame = true;

    if (mInfo->mIsPlayerIt) {
        mModeTimer->disableTimer();
        mModeLayout->showSeeking();
    } else {
        mModeTimer->enableTimer();
        mModeLayout->showHiding();
    }

    CoinCounter*                   coinCollect  = mCurScene->mSceneLayout->mCoinCollectLyt;
    CoinCounter*                   coinCounter  = mCurScene->mSceneLayout->mCoinCountLyt;
    MapMini*                       compass      = mCurScene->mSceneLayout->mMapMiniLyt;
    al::SimpleLayoutAppearWaitEnd* playGuideLyt = mCurScene->mSceneLayout->mPlayGuideMenuLyt;

    mInvulnTime = 0;

    if (coinCounter->mIsAlive)  { coinCounter->tryEnd(); }
    if (coinCollect->mIsAlive)  { coinCollect->tryEnd(); }
    if (compass->mIsAlive)      { compass->end();        }
    if (playGuideLyt->mIsAlive) { playGuideLyt->end();   }

    GameModeBase::begin();

    Client::sendTagInfPacket();
}

void HideAndSeekMode::end() {

    mModeLayout->tryEnd();

    mModeTimer->disableTimer();

    CoinCounter*                   coinCollect  = mCurScene->mSceneLayout->mCoinCollectLyt;
    CoinCounter*                   coinCounter  = mCurScene->mSceneLayout->mCoinCountLyt;
    MapMini*                       compass      = mCurScene->mSceneLayout->mMapMiniLyt;
    al::SimpleLayoutAppearWaitEnd* playGuideLyt = mCurScene->mSceneLayout->mPlayGuideMenuLyt;

    mInvulnTime = 0.0f;

    if (!coinCounter->mIsAlive)  { coinCounter->tryStart();  }
    if (!coinCollect->mIsAlive)  { coinCollect->tryStart();  }
    if (!compass->mIsAlive)      { compass->appearSlideIn(); }
    if (!playGuideLyt->mIsAlive) { playGuideLyt->appear();   }

    GameModeBase::end();

    Client::sendTagInfPacket();
}

void HideAndSeekMode::update() {
    PlayerActorBase* playerBase = rs::getPlayerActor(mCurScene);

    bool isYukimaru = !playerBase->getPlayerInfo(); // if PlayerInfo is a nullptr, that means we're dealing with the bound bowl racer

    if (mIsFirstFrame) {
        if (mInfo->mIsUseGravityCam && mTicket) {
            al::startCamera(mCurScene, mTicket, -1);
        }
        mIsFirstFrame = false;
    }

    if (!mInfo->mIsPlayerIt) {
        if (mInvulnTime < 5) {
            mInvulnTime += Time::deltaTime;
        } else if (playerBase) {
            for (size_t i = 0; i < (size_t)mPuppetHolder->getSize(); i++) {
                PuppetInfo* other = Client::getPuppetInfo(i);
                if (!other) {
                    Logger::log("Checking %d, hit bounds %d-%d\n", i, mPuppetHolder->getSize(), Client::getMaxPlayerCount());
                    break;
                }

                if (!other->isConnected || !other->isInSameStage || !other->isIt || isYukimaru) {
                    continue;
                }

                float pupDist = al::calcDistance(playerBase, other->playerPos); // TODO: remove distance calculations and use hit sensors to determine this

                if (pupDist < 200.f && ((PlayerActorHakoniwa*)playerBase)->mDimKeeper->is2DModel == other->is2D) {
                    if (!PlayerFunction::isPlayerDeadStatus(playerBase)) {
                        GameDataFunction::killPlayer(GameDataHolderAccessor(this));
                        playerBase->startDemoPuppetable();
                        al::setVelocityZero(playerBase);
                        rs::faceToCamera(playerBase);
                        ((PlayerActorHakoniwa*)playerBase)->mPlayerAnimator->endSubAnim();
                        ((PlayerActorHakoniwa*)playerBase)->mPlayerAnimator->startAnimDead();

                        updateTagState(true);
                    }
                } else if (PlayerFunction::isPlayerDeadStatus(playerBase)) {
                    updateTagState(true);
                }
            }
        }

        mModeTimer->updateTimer();
    }

    // Gravity
    if (mInfo->mIsUseGravity && !isYukimaru) {
        sead::Vector3f gravity;
        if (rs::calcOnGroundNormalOrGravityDir(&gravity, playerBase, playerBase->getPlayerCollision())) {
            gravity = -gravity;
            al::normalize(&gravity);
            al::setGravity(playerBase, gravity);
            al::setGravity(((PlayerActorHakoniwa*)playerBase)->mHackCap, gravity);
        }

        if (al::isPadHoldL(-1)) {
            if (al::isPadTriggerRight(-1)) {
                if (al::isActiveCamera(mTicket)) {
                    al::endCamera(mCurScene, mTicket, -1, false);
                    mInfo->mIsUseGravityCam = false;
                } else {
                    al::startCamera(mCurScene, mTicket, -1);
                    mInfo->mIsUseGravityCam = true;
                }
            }
        } else if (al::isPadTriggerZL(-1) && al::isPadTriggerLeft(-1)) {
            killMainPlayer(((PlayerActorHakoniwa*)playerBase));
        }
    }

    // Switch roles
    if (al::isPadTriggerUp(-1) && !al::isPadHoldZL(-1)) {
        updateTagState(!mInfo->mIsPlayerIt);
    }

    mInfo->mHidingTime = mModeTimer->getTime();
}

void HideAndSeekMode::updateTagState(bool isSeeking) {
    mInfo->mIsPlayerIt = isSeeking;

    if (isSeeking) {
        mModeTimer->disableTimer();
        mModeLayout->showSeeking();
    } else {
        mModeTimer->enableTimer();
        mModeLayout->showHiding();
        mInvulnTime = 0;
    }

    Client::sendTagInfPacket();
}
