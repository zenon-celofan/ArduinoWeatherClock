#pragma once

#include <WString.h>
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

bool connectToWiFi(const String &ssid, const String &password,
                   unsigned long &lastWifiConnectedAt,
                   bool &wifiWasEverConnected,
                   unsigned long &wifiDisconnectedSince);
