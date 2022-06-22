#include "server/sardines/SardineConfigMenu.hpp"
#include "logger.hpp"
#include "server/Client.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include <cmath>

SardineConfigMenu::SardineConfigMenu() : GameModeConfigMenu() {
    mItems = new sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>();
    mItems->mBuffer[0].copy(u"Sardine Gravity (OFF)"); // TBD
    mItems->mBuffer[1].copy(u"Sardine Tether (OFF)");  // TBD
    mItems->mBuffer[2].copy(u"Tether Snapping (OFF)"); // TBD
}

const sead::WFixedSafeString<0x200>* SardineConfigMenu::getStringData() {
    mItems->mBuffer[0].copy(
        SardineInfo::mIsUseGravity
        ? u"Sardine Gravity (ON) "
        : u"Sardine Gravity (OFF)"
    );
    mItems->mBuffer[1].copy(
        SardineInfo::mIsTether
        ? u"Sardine Tether (ON) "
        : u"Sardine Tether (OFF)"
    );
    mItems->mBuffer[2].copy(
        SardineInfo::mIsTether && SardineInfo::mIsTetherSnap
        ? u"Tether Snapping (ON) "
        : u"Tether Snapping (OFF)"
    );

    return mItems->mBuffer;
}

GameModeConfigMenu::UpdateAction SardineConfigMenu::updateMenu(int selectIndex) {
    switch (selectIndex) {
        case 0: {
            Logger::log("Setting Gravity Mode.\n");
            SardineInfo::mIsUseGravity = !SardineInfo::mIsUseGravity;
            return UpdateAction::REFRESH;
        }
        case 1: {
            Logger::log("Setting Sardine Tether.\n");
            SardineInfo::mIsTether = !SardineInfo::mIsTether;
            return UpdateAction::REFRESH;
        }
        case 2: {
            Logger::log("Setting Sardine Tether Snap.\n");
            SardineInfo::mIsTetherSnap = SardineInfo::mIsTether && !SardineInfo::mIsTetherSnap;
            return UpdateAction::REFRESH;
        }
        default: {
            Logger::log("Failed to interpret Index!\n");
            return UpdateAction::NOOP;
        }
    }
}
