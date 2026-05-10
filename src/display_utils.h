#pragma once

#include <WString.h>

struct DisplayDecision {
    char text[8];
    bool centered;
};

DisplayDecision decideDisplayContent(
    bool apMode,
    bool wifiWasEverConnected,
    unsigned long wifiDisconnectedSince,
    unsigned long currentMillis,
    int displayMode,
    bool showTime,
    int hour,
    int minute,
    float localtemp
);
