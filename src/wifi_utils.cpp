#include "wifi_utils.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>

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

bool connectToWiFi(const String &ssid, const String &password,
                   unsigned long &lastWifiConnectedAt,
                   bool &wifiWasEverConnected,
                   unsigned long &wifiDisconnectedSince) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.print("Connecting to WiFi");

    for (int i = 0; i < 20; i++) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nConnected!");
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
            lastWifiConnectedAt = millis();
            wifiWasEverConnected = true;
            wifiDisconnectedSince = 0;
            return true;
        }
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nFailed to connect.");
    if (wifiDisconnectedSince == 0) wifiDisconnectedSince = millis();
    return false;
}
