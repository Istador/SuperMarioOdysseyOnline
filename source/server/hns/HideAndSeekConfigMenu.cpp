#include "server/hns/HideAndSeekConfigMenu.hpp"
#include <cmath>
#include "logger.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/hns/HideAndSeekMode.hpp"
#include "server/Client.hpp"

HideAndSeekConfigMenu::HideAndSeekConfigMenu() : GameModeConfigMenu() {
    mItems = new sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>();
    mItems->mBuffer[0].copy(u"Toggle H&S Gravity (OFF)"); // TBD
}

void HideAndSeekConfigMenu::initMenu(const al::LayoutInitInfo &initInfo) {}

const sead::WFixedSafeString<0x200>* HideAndSeekConfigMenu::getStringData() {
    HideAndSeekInfo* curMode = GameModeManager::instance()->getInfo<HideAndSeekInfo>();

    mItems->mBuffer[0].copy(
        GameModeManager::instance()->isMode(GameMode::HIDEANDSEEK)
        && curMode != nullptr
        && curMode->mIsUseGravity
        ? u"Toggle H&S Gravity (ON) "
        : u"Toggle H&S Gravity (OFF)"
    );

    return mItems->mBuffer;
}

GameModeConfigMenu::UpdateAction HideAndSeekConfigMenu::updateMenu(int selectIndex) {
    Logger::log("Setting Gravity Mode.\n");

    switch (selectIndex) {
        case 0: {
            HideAndSeekInfo* curMode = GameModeManager::instance()->getInfo<HideAndSeekInfo>();
            if (!curMode) {
                Logger::log("Unable to Load Mode info!\n");
                return UpdateAction::NOOP;
            }
            if (GameModeManager::instance()->isMode(GameMode::HIDEANDSEEK)) {
                curMode->mIsUseGravity = !curMode->mIsUseGravity;
                return UpdateAction::REFRESH;
            }
            return UpdateAction::NOOP;
        }
        default:
            Logger::log("Failed to interpret Index!\n");
            return UpdateAction::NOOP;
    }
}
