#pragma once

#include "server/gamemode/GameModeConfigMenu.hpp"
#include "game/Layouts/CommonVerticalList.h"
#include "server/gamemode/GameModeBase.hpp"
#include "server/sardines/SardineInfo.hpp"

class SardineConfigMenu : public GameModeConfigMenu {
    public:
        SardineConfigMenu();

        const sead::WFixedSafeString<0x200>* getStringData() override;
        GameModeConfigMenu::UpdateAction updateMenu(int selectIndex) override;

        const int getMenuSize() override { return SardineInfo::mIsTether ? 3 : 2; }

    private:
        static constexpr int mItemCount = 3;
        sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>* mItems = nullptr;
};
