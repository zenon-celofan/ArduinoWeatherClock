#pragma once

#include <WString.h>

struct DisplayDecision {
    char text[8];
    bool centered;
};

bool detectDisplayChange(const char *newText, String &lastContent);

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
