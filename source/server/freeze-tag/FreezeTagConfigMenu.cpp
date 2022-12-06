#include "server/freeze-tag/FreezeTagConfigMenu.hpp"
#include <cmath>
#include <stdint.h>
#include "Keyboard.hpp"
#include "logger.hpp"
#include "server/gamemode/GameMode.hpp"
#include "server/gamemode/GameModeManager.hpp"

FreezeTagConfigMenu::FreezeTagConfigMenu() : GameModeConfigMenu() {
    mItems = new sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>();
    mItems->mBuffer[0].copy(u"Host Mode (OFF)"); // TBD
    mItems->mBuffer[1].copy(u"Debug Mode (OFF)"); // TBD
    mItems->mBuffer[2].copy(u"Set Score");
    mItems->mBuffer[3].copy(u"Set Round Duration");

    mScoreKeyboard = new Keyboard(6);
    mScoreKeyboard->setHeaderText(u"Set your Freeze Tag score");
    mScoreKeyboard->setSubText(u"");

    mRoundKeyboard = new Keyboard(3);
    mRoundKeyboard->setHeaderText(u"Set length of rounds you start in minutes");
    mRoundKeyboard->setSubText(u"This will be automatically sent to other players (2-60 minutes)");
}

const sead::WFixedSafeString<0x200>* FreezeTagConfigMenu::getStringData() {
    mItems->mBuffer[0].copy(
        FreezeTagInfo::mIsHostMode
        ? u"Host Mode (ON) "
        : u"Host Mode (OFF)"
    );

    mItems->mBuffer[1].copy(
        FreezeTagInfo::mIsDebugMode
        ? u"Debug Mode (ON) "
        : u"Debug Mode (OFF)"
    );

    return mItems->mBuffer;
}

GameModeConfigMenu::UpdateAction FreezeTagConfigMenu::updateMenu(int selectIndex) {
    switch (selectIndex) {
        // Toggle Host Mode
        case 0: {
            FreezeTagInfo::mIsHostMode = !FreezeTagInfo::mIsHostMode;
            return UpdateAction::REFRESH;
        }
        // Toggle Debug Mode
        case 1: {
            FreezeTagInfo::mIsDebugMode = !FreezeTagInfo::mIsDebugMode;
            return UpdateAction::REFRESH;
        }
        // Set Score
        case 2: {
            uint16_t oldScore = FreezeTagInfo::mPlayerTagScore.mScore;
            uint16_t newScore = -1;

            char buf[5];
            nn::util::SNPrintf(buf, 5, "%u", oldScore);

            mScoreKeyboard->openKeyboard(
                buf,
                [](nn::swkbd::KeyboardConfig& config) {
                    config.keyboardMode  = nn::swkbd::KeyboardMode::ModeNumeric;
                    config.textMaxLength = 4;
                    config.textMinLength = 1;
                    config.isUseUtf8     = true;
                    config.inputFormMode = nn::swkbd::InputFormMode::OneLine;
                }
            );

            while (!mScoreKeyboard->isThreadDone()) {
                nn::os::YieldThread(); // allow other threads to run
            }

            if (!mScoreKeyboard->isKeyboardCancelled()) {
                newScore = ::atoi(mScoreKeyboard->getResult());
            }

            if (newScore != uint16_t(-1)) {
                FreezeTagInfo::mPlayerTagScore.mScore = newScore;
            }

            // We don't need to send the new score to other players here, because
            //   FreezeTagMode::update() checks for score changes every iteration

            return UpdateAction::NOOP;
        }
        // Set Round Duration
        case 3: {
            uint8_t oldTime = FreezeTagInfo::mRoundLength;
            uint8_t newTime = -1;

            char buf[3];
            nn::util::SNPrintf(buf, 3, "%u", oldTime);

            mRoundKeyboard->openKeyboard(
                buf,
                [](nn::swkbd::KeyboardConfig& config) {
                    config.keyboardMode  = nn::swkbd::KeyboardMode::ModeNumeric;
                    config.textMaxLength = 2;
                    config.textMinLength = 1;
                    config.isUseUtf8     = true;
                    config.inputFormMode = nn::swkbd::InputFormMode::OneLine;
                }
            );

            while (!mRoundKeyboard->isThreadDone()) {
                nn::os::YieldThread(); // allow other threads to run
            }

            if (!mRoundKeyboard->isKeyboardCancelled()) {
                newTime = ::atoi(mRoundKeyboard->getResult());
            }

            if (newTime != uint8_t(-1)) {
                FreezeTagInfo::mRoundLength = al::clamp(newTime, u8(2), u8(60));
            }
            return UpdateAction::NOOP;
        }
        default: {
            Logger::log("Failed to interpret Index!\n");
            return UpdateAction::NOOP;
        }
    }
}
