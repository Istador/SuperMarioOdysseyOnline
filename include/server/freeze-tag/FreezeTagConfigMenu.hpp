#pragma once

#include "Keyboard.hpp"
#include "al/layout/LayoutInitInfo.h"
#include "container/seadSafeArray.h"
#include "prim/seadSafeString.h"
#include "server/gamemode/GameModeConfigMenu.hpp"
#include "server/freeze-tag/FreezeTagInfo.h"

class FreezeTagConfigMenu : public GameModeConfigMenu {
    public:
        FreezeTagConfigMenu();

        const sead::WFixedSafeString<0x200>* getStringData() override;
        GameModeConfigMenu::UpdateAction updateMenu(int selectIndex) override;

        const int getMenuSize() override { return FreezeTagInfo::mIsHostMode ? 4 : 3; }

    private:
        static constexpr int mItemCount = 4;
        sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>* mItems = nullptr;
        Keyboard* mScoreKeyboard;
        Keyboard* mRoundKeyboard;
};
