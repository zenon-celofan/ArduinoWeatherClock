#include "wifi_utils.h"

WifiEvent evaluateWifiConnection(bool isConnected, const WifiState &state, unsigned long now, unsigned long *outDurationSec) {
    if (state.wasEverConnected && !isConnected) {
        if (state.wifiDisconnectedSince == 0) {
            return WIFI_FIRST_LOST;
        }
        return WIFI_NONE;
    }

    if (isConnected && state.wifiDisconnectedSince > 0) {
        if (outDurationSec) {
            *outDurationSec = (now - state.wifiDisconnectedSince) / 1000UL;
        }
        return WIFI_RECONNECTED;
    }

    return WIFI_NONE;
}

WifiEvent evaluateReconnectWifi(bool isConnected, unsigned long wifiDisconnectedSince, unsigned long now, unsigned long *outDurationSec) {
    if (!isConnected) {
        if (wifiDisconnectedSince == 0) {
            return WIFI_FIRST_LOST;
        }
        return WIFI_STILL_LOST;
    }

    if (wifiDisconnectedSince > 0) {
        if (outDurationSec) {
            *outDurationSec = (now - wifiDisconnectedSince) / 1000UL;
        }
        return WIFI_RECONNECTED;
    }

    return WIFI_NONE;
}
