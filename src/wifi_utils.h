#pragma once

#include <cstdint>

enum WifiEvent : uint8_t {
    WIFI_NONE,
    WIFI_FIRST_LOST,
    WIFI_STILL_LOST,
    WIFI_RECONNECTED,
};

struct WifiState {
    unsigned long wifiDisconnectedSince;
    unsigned long lastWifiConnectedAt;
    bool wasEverConnected;
};

WifiEvent evaluateWifiConnection(bool isConnected, const WifiState &state, unsigned long now, unsigned long *outDurationSec);

WifiEvent evaluateReconnectWifi(bool isConnected, unsigned long wifiDisconnectedSince, unsigned long now, unsigned long *outDurationSec);
