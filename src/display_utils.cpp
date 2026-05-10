#include "display_utils.h"
#include "temp_utils.h"
#include <cstdio>
#include <cstring>

bool detectDisplayChange(const char *newText, String &lastContent) {
    String newStr(newText);
    if (lastContent != newStr) {
        lastContent = newStr;
        return true;
    }
    return false;
}

static bool computeWifiStale(
    bool wifiWasEverConnected,
    unsigned long wifiDisconnectedSince,
    unsigned long currentMillis
) {
    return wifiWasEverConnected && wifiDisconnectedSince > 0
        && (currentMillis - wifiDisconnectedSince) > 60000UL;
}

static void formatTime(char *buf, int hour, int minute) {
    snprintf(buf, 8, "%d%02d", hour, minute);
}

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
) {
    DisplayDecision d = {"", true};

    if (apMode) {
        snprintf(d.text, sizeof(d.text), "AP");
        d.centered = true;
        return d;
    }

    bool stale = computeWifiStale(wifiWasEverConnected, wifiDisconnectedSince, currentMillis);

    if (stale) {
        unsigned long cycle = (currentMillis - wifiDisconnectedSince) % 10000UL;
        if (cycle < 1000) {
            snprintf(d.text, sizeof(d.text), "wifi");
            d.centered = true;
        } else {
            formatTime(d.text, hour, minute);
            d.centered = false;
        }
        return d;
    }

    if (displayMode == 1 || (displayMode == 0 && showTime)) {
        formatTime(d.text, hour, minute);
        d.centered = false;
    } else {
        String tempStr = formatTemperature(localtemp);
        snprintf(d.text, sizeof(d.text), "%s", tempStr.c_str());
        d.centered = true;
    }

    return d;
}
