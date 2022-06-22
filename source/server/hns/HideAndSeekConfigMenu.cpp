#include "server/hns/HideAndSeekConfigMenu.hpp"
#include <cmath>
#include "logger.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/hns/HideAndSeekInfo.hpp"
#include "server/Client.hpp"

HideAndSeekConfigMenu::HideAndSeekConfigMenu() : GameModeConfigMenu() {
    mItems = new sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>();
    mItems->mBuffer[0].copy(u"Toggle H&S Gravity (OFF)"); // TBD
}

const sead::WFixedSafeString<0x200>* HideAndSeekConfigMenu::getStringData() {
    mItems->mBuffer[0].copy(
        HideAndSeekInfo::mIsUseGravity
        ? u"Toggle H&S Gravity (ON) "
        : u"Toggle H&S Gravity (OFF)"
    );

    return mItems->mBuffer;
}

GameModeConfigMenu::UpdateAction HideAndSeekConfigMenu::updateMenu(int selectIndex) {
    switch (selectIndex) {
        case 0: {
            Logger::log("Setting Gravity Mode.\n");
            HideAndSeekInfo::mIsUseGravity = !HideAndSeekInfo::mIsUseGravity;
            return UpdateAction::REFRESH;
        }
        default: {
            Logger::log("Failed to interpret Index!\n");
            return UpdateAction::NOOP;
        }
    }
}
