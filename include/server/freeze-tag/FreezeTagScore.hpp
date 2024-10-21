#pragma once

#include <stdint.h>
#include "server/freeze-tag/FreezeTagIcon.h"

class FreezeTagScore {
    public:
        uint16_t mScore     = 0;
        uint16_t mPrevScore = 0;

        void setTargetLayout(FreezeTagIcon* icon) { mIcon = icon; };

        void resetScore() { mScore = 0; };

        // Events
        void eventScoreDebug()        { addScore( 1, "Debugging!");    }
        void eventScoreSurvivalTime() { addScore( 1, "Survival Time"); }
        void eventScoreUnfreeze()     { addScore( 2, "Rescued");       }
        void eventScoreFreeze()       { addScore( 2, "Caught");        }
        void eventScoreFallOff()      { addScore( 2, "Fell Off L");    }
        void eventScoreRunnerWin()    { addScore( 4, "Survived!");     }
        void eventScoreWipeout()      { addScore(20, "Wipeout!");      }

        void eventNotEnoughRunnersToStart() { addScore( 0, "Not enough Runners to start a round"); }
        void eventNotEnoughChasersToStart() { addScore( 0, "Not enough Chasers to start a round"); }

    private:
        FreezeTagIcon* mIcon;

        void addScore(int add, const char* description) {
            mScore += add;
            mScore  = al::clamp(mScore, uint16_t(0), uint16_t(9999));
            mIcon->queueScoreEvent(add, description);
        };
};
