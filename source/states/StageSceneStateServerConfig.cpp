#include "game/StageScene/StageSceneStateServerConfig.hpp"
#include <cstdlib>
#include <cstring>
#include <math.h>
#include "al/string/StringTmp.h"
#include "basis/seadNew.h"
#include "game/Layouts/CommonVerticalList.h"
#include "game/SaveData/SaveDataAccessFunction.h"
#include "server/Client.hpp"
#include "al/util.hpp"
#include "al/util/NerveUtil.h"
#include "container/seadPtrArray.h"
#include "container/seadSafeArray.h"
#include "logger.hpp"
#include "prim/seadSafeString.h"
#include "prim/seadStringUtil.h"
#include "rs/util/InputUtil.h"
#include "server/gamemode/GameModeBase.hpp"
#include "server/gamemode/GameModeConfigMenuFactory.hpp"
#include "server/gamemode/GameModeFactory.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/hns/HideAndSeekMode.hpp"

// WIP work on RollPartsData, not exactly working out atm
const char16_t* testValues[] = {u"Test 1", u"Test 2", u"Test 3", u"Test 4", u"Test 5",
                                u"Test 6", u"Test 7", u"Test 8", u"Test 9"};

StageSceneStateServerConfig::StageSceneStateServerConfig(const char *name, al::Scene *scene, const al::LayoutInitInfo &initInfo, FooterParts *footerParts, GameDataHolder *dataHolder, bool) : al::HostStateBase<al::Scene>(name, scene) {
    mFooterParts = footerParts;
    mGameDataHolder = dataHolder;

    mMsgSystem = initInfo.getMessageSystem();

    mInput = new InputSeparator(mHost, true);

    // page 0 menu
    mMainOptions = new SimpleLayoutMenu("ServerConfigMenu", "OptionSelect", initInfo, 0, false);
    mMainOptionsList = new CommonVerticalList(mMainOptions, initInfo, true);

    al::setPaneString(mMainOptions, "TxtOption", u"Server Configuration", 0);

    mMainOptionsList->unkInt1 = 1;

    mMainOptionsList->initDataNoResetSelected(4);

    sead::SafeArray<sead::WFixedSafeString<0x200>, 4>* mainMenuOptions =
        new sead::SafeArray<sead::WFixedSafeString<0x200>, 4>();

    mainMenuOptions->mBuffer[ServerConfigOption::GAMEMODECONFIG].copy(u"Gamemode Config");
    mainMenuOptions->mBuffer[ServerConfigOption::GAMEMODESWITCH].copy(u"Change Gamemode");
    mainMenuOptions->mBuffer[ServerConfigOption::SETIP].copy(u"Change Server IP");
    mainMenuOptions->mBuffer[ServerConfigOption::SETPORT].copy(u"Change Server Port");

    mMainOptionsList->addStringData(mainMenuOptions->mBuffer, "TxtContent");

    // WIP work on RollPartsData, not exactly working out atm
    // RollPartsData* testArr = new RollPartsData[2]();

    // for (int i = 0; i < 2; i++) {
    //     RollPartsData* part = &testArr[i];

    //     part->mRollMsgCount = 3;
    //     part->mRollMsgList = testValues;
    //     part->unkInt1 = 0;
    //     part->mUnkBool = i == 0;
    // }

    //mMainOptionsList->startLoopActionAll("Loop", "Loop");
    
    // mMainOptionsList->setRollPartsData(testArr);

    // gamemode select menu

    mModeSelect = new SimpleLayoutMenu("GamemodeSelectMenu", "OptionSelect", initInfo, 0, false);
    mModeSelectList = new CommonVerticalList(mModeSelect, initInfo, true);

    al::setPaneString(mModeSelect, "TxtOption", u"Gamemode Selection", 0);

    const int modeCount = GameModeFactory::getModeCount();

    mModeSelectList->initDataNoResetSelected(modeCount);

    sead::SafeArray<sead::WFixedSafeString<0x200>, modeCount>* modeSelectOptions =
        new sead::SafeArray<sead::WFixedSafeString<0x200>, modeCount>();

    for (size_t i = 0; i < modeCount; i++) {
        const char* modeName = GameModeFactory::getModeName(i);
        modeSelectOptions->mBuffer[i].convertFromMultiByteString(modeName, strlen(modeName));
    }

    mModeSelectList->addStringData(modeSelectOptions->mBuffer, "TxtContent");

    // gamemode config menu
    GameModeConfigMenuFactory factory("GameModeConfigFactory");
    for (int mode = 0; mode < factory.getMenuCount(); mode++) {
        GameModeEntry& entry = mGamemodeConfigMenus[mode];
        const char* name = factory.getMenuName(mode);
        entry.mMenu = factory.getCreator(name)(name);
        entry.mLayout = new SimpleLayoutMenu("GameModeConfigMenu", "OptionSelect", initInfo, 0, false);
        entry.mList = new CommonVerticalList(entry.mLayout, initInfo, true);

        al::setPaneString(entry.mLayout, "TxtOption", u"Gamemode Configuration", 0);
    }


    mCurrentList = mMainOptionsList;
    mCurrentMenu = mMainOptions;
}

void StageSceneStateServerConfig::init() {
    initNerve(&nrvStageSceneStateServerConfigMainMenu, 0);

    #ifdef EMU
    char ryujinx[0x10] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    nn::account::Uid user;
    nn::account::GetLastOpenedUser(&user);
    if (memcmp(user.data, ryujinx, 0x10) == 0) {
        Client::showUIMessage(u"Error: Ryujinx default profile detected.\nYou have to create a new user profile!");
    }
    #endif
}

void StageSceneStateServerConfig::appear(void) {
    mCurrentMenu->startAppear("Appear");
    al::NerveStateBase::appear();
}

void StageSceneStateServerConfig::kill(void) {
    mCurrentMenu->startEnd("End");
    al::NerveStateBase::kill();
}

al::MessageSystem* StageSceneStateServerConfig::getMessageSystem(void) const {
    return mMsgSystem;
}

void StageSceneStateServerConfig::exeMainMenu() {
    if (al::isFirstStep(this)) {
        activateInput();
    }

    mInput->update();

    mCurrentList->update();

    if (mInput->isTriggerUiUp()) {
        mCurrentList->up();    }

    if (mInput->isTriggerUiDown()) {
        mCurrentList->down();
    }

    if (rs::isTriggerUiCancel(mHost)) {
        kill();
    }

    if (rs::isTriggerUiDecide(mHost)) {
        deactivateInput();
    }

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        switch (mCurrentList->mCurSelected) {
        case ServerConfigOption::GAMEMODECONFIG: {
            al::setNerve(this, &nrvStageSceneStateServerConfigGamemodeConfig);
            break;
        }
        case ServerConfigOption::GAMEMODESWITCH: {
            al::setNerve(this, &nrvStageSceneStateServerConfigGamemodeSelect);
            break;
        }
        case ServerConfigOption::SETIP: {
            al::setNerve(this, &nrvStageSceneStateServerConfigOpenKeyboardIP);
            break;
        }
        case ServerConfigOption::SETPORT: {
            al::setNerve(this, &nrvStageSceneStateServerConfigOpenKeyboardPort);
            break;
        }
        default:
            kill();
            break;
        }
    }
}

void StageSceneStateServerConfig::exeOpenKeyboardIP() {
    if (al::isFirstStep(this)) {

        mCurrentList->deactivate();

        Client::getKeyboard()->setHeaderText(u"Set a Server IP Below.");
        Client::getKeyboard()->setSubText(u"");

        bool isSave = Client::openKeyboardIP(); // anything that happens after this will be ran after the keyboard closes
        
        al::startHitReaction(mCurrentMenu, "リセット", 0);

        if(isSave) 
            al::setNerve(this, &nrvStageSceneStateServerConfigSaveData);
        else
            al::setNerve(this, &nrvStageSceneStateServerConfigMainMenu);
    }
}

void StageSceneStateServerConfig::exeOpenKeyboardPort() {
    if (al::isFirstStep(this)) {

        mCurrentList->deactivate();

        Client::getKeyboard()->setHeaderText(u"Set a Server Port Below.");
        Client::getKeyboard()->setSubText(u"");

        bool isSave = Client::openKeyboardPort(); // anything that happens after this will be ran after the keyboard closes

        al::startHitReaction(mCurrentMenu, "リセット", 0);
        
        if(isSave) 
            al::setNerve(this, &nrvStageSceneStateServerConfigSaveData);
        else
            al::setNerve(this, &nrvStageSceneStateServerConfigMainMenu);
    }
}

void StageSceneStateServerConfig::exeGamemodeConfig() {
    if (al::isFirstStep(this)) {
        mGamemodeConfigMenu = &mGamemodeConfigMenus[GameModeManager::instance()->getGameMode()];

        mGamemodeConfigMenu->mList->initDataNoResetSelected(mGamemodeConfigMenu->mMenu->getMenuSize());
        mGamemodeConfigMenu->mList->addStringData(mGamemodeConfigMenu->mMenu->getStringData(), "TxtContent");

        mCurrentList = mGamemodeConfigMenu->mList;
        mCurrentMenu = mGamemodeConfigMenu->mLayout;

        subMenuStart();
    }

    subMenuUpdate();

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        GameModeConfigMenu::UpdateAction action = mGamemodeConfigMenu->mMenu->updateMenu(mCurrentList->mCurSelected);
        switch (action) {
            case GameModeConfigMenu::UpdateAction::CLOSE:
                endSubMenu();
                break;
            case GameModeConfigMenu::UpdateAction::REFRESH:
                subMenuRefresh();
                break;
            case GameModeConfigMenu::UpdateAction::NOOP:
                activateInput();
                break;
        }
    }
}

void StageSceneStateServerConfig::exeGamemodeSelect() {
    if (al::isFirstStep(this)) {
        mCurrentList = mModeSelectList;
        mCurrentMenu = mModeSelect;

        subMenuStart();
    }

    subMenuUpdate();

    if (mIsDecideConfig && mCurrentList->isDecideEnd()) {
        Logger::log("Setting Server Mode to: %d\n", mCurrentList->mCurSelected);
        GameModeManager::instance()->setMode(static_cast<GameMode>(mCurrentList->mCurSelected));
        endSubMenu();
    }
}

void StageSceneStateServerConfig::exeSaveData() {
    if (al::isFirstStep(this)) {
        SaveDataAccessFunction::startSaveDataWrite(mGameDataHolder);
    }

    if (SaveDataAccessFunction::updateSaveDataAccess(mGameDataHolder, false)) {
        al::startHitReaction(mCurrentMenu, "リセット", 0);
        al::setNerve(this, &nrvStageSceneStateServerConfigMainMenu);
    }
}

void StageSceneStateServerConfig::endSubMenu() {
    mCurrentList->deactivate();
    mCurrentMenu->kill();

    mCurrentList = mMainOptionsList;
    mCurrentMenu = mMainOptions;

    mCurrentMenu->startAppear("Appear");

    al::startHitReaction(mCurrentMenu, "リセット", 0);
    al::setNerve(this, &nrvStageSceneStateServerConfigMainMenu);
}

void StageSceneStateServerConfig::subMenuStart() {
    mCurrentList->deactivate();

    mCurrentMenu->kill();

    activateInput();

    mCurrentMenu->startAppear("Appear");
}

void StageSceneStateServerConfig::subMenuUpdate() {
    mInput->update();

    mCurrentList->update();

    if (mInput->isTriggerUiUp()) {
        mCurrentList->up();
    }

    if (mInput->isTriggerUiDown()) {
        mCurrentList->down();
    }

    if (rs::isTriggerUiCancel(mHost)) {
        endSubMenu();
    }

    if (rs::isTriggerUiDecide(mHost)) {
        deactivateInput();
    }
}

void StageSceneStateServerConfig::subMenuRefresh() {
    mGamemodeConfigMenu = &mGamemodeConfigMenus[GameModeManager::instance()->getGameMode()];
    mGamemodeConfigMenu->mList->initDataNoResetSelected(mGamemodeConfigMenu->mMenu->getMenuSize());
    mGamemodeConfigMenu->mList->addStringData(mGamemodeConfigMenu->mMenu->getStringData(), "TxtContent");
    mGamemodeConfigMenu->mList->updateParts();
    activateInput();
}

void StageSceneStateServerConfig::activateInput() {
    mInput->reset();
    mCurrentList->activate();
    mCurrentList->appearCursor();
    mIsDecideConfig = false;
}

void StageSceneStateServerConfig::deactivateInput() {
    al::startHitReaction(mCurrentMenu, "決定", 0);
    mCurrentList->endCursor();
    mCurrentList->decide();
    mIsDecideConfig = true;
}

namespace {
NERVE_IMPL(StageSceneStateServerConfig, MainMenu)
NERVE_IMPL(StageSceneStateServerConfig, OpenKeyboardIP)
NERVE_IMPL(StageSceneStateServerConfig, OpenKeyboardPort)
NERVE_IMPL(StageSceneStateServerConfig, GamemodeConfig)
NERVE_IMPL(StageSceneStateServerConfig, GamemodeSelect)
NERVE_IMPL(StageSceneStateServerConfig, SaveData)
}
