#include "wifi_utils.h"
#include "ESP8266WiFi.h"
#include "Arduino.h"
#include <cstdio>

#define RUN_TEST(name, expr) do {                                    \
    printf("  TEST: %s ... ", name);                                 \
    if (!(expr)) { printf("FAIL\n"); return 1; }                     \
    printf("PASS\n");                                                \
} while(0)

int main() {
    puts("\n=== evaluateWifiConnection Tests ===\n");

    unsigned long dur = 0;

    // --- wasEverConnected=false: not yet connected, no loss detection ---
    {
        WifiState s = {0, 0, false};
        RUN_TEST("never connected, not connected -> NONE",
            evaluateWifiConnection(false, s, 1000, &dur) == WIFI_NONE);
    }
    {
        WifiState s = {0, 0, false};
        RUN_TEST("never connected, now connected -> NONE (not a reconnect)",
            evaluateWifiConnection(true, s, 1000, &dur) == WIFI_NONE);
    }

    // --- Connected, no prior loss -> NONE ---
    {
        WifiState s = {0, 500, true};
        RUN_TEST("connected, disconnectedSince=0 -> NONE",
            evaluateWifiConnection(true, s, 2000, &dur) == WIFI_NONE);
    }

    // --- Just lost, first detection -> FIRST_LOST ---
    {
        WifiState s = {0, 500, true};
        RUN_TEST("first loss detection -> FIRST_LOST",
            evaluateWifiConnection(false, s, 1000, &dur) == WIFI_FIRST_LOST);
    }

    // --- Still lost, already detected -> NONE ---
    {
        WifiState s = {1000, 500, true};
        RUN_TEST("still lost, already logged -> NONE",
            evaluateWifiConnection(false, s, 5000, &dur) == WIFI_NONE);
    }

    // --- Reconnected after loss -> RECONNECTED ---
    {
        WifiState s = {1000, 500, true};
        WifiEvent ev = evaluateWifiConnection(true, s, 15000, &dur);
        RUN_TEST("reconnected -> RECONNECTED", ev == WIFI_RECONNECTED);
        RUN_TEST("reconnected duration 14s", dur == 14);
    }

    // --- Reconnected after exactly 1s ---
    {
        WifiState s = {5000, 500, true};
        WifiEvent ev = evaluateWifiConnection(true, s, 6000, &dur);
        RUN_TEST("reconnected after 1s", ev == WIFI_RECONNECTED);
        RUN_TEST("duration 1s", dur == 1);
    }

    // --- Null duration pointer (no crash) ---
    {
        WifiState s = {1000, 500, true};
        RUN_TEST("null duration ptr -> RECONNECTED",
            evaluateWifiConnection(true, s, 5000, NULL) == WIFI_RECONNECTED);
    }

    puts("\n=== evaluateReconnectWifi Tests ===\n");

    // --- Connected, no loss -> NONE ---
    {
        RUN_TEST("reconnect: connected, no loss -> NONE",
            evaluateReconnectWifi(true, 0, 1000, &dur) == WIFI_NONE);
    }

    // --- Disconnected, first time -> FIRST_LOST ---
    {
        RUN_TEST("reconnect: first time lost -> FIRST_LOST",
            evaluateReconnectWifi(false, 0, 1000, &dur) == WIFI_FIRST_LOST);
    }

    // --- Disconnected, already recorded -> STILL_LOST ---
    {
        RUN_TEST("reconnect: still lost -> STILL_LOST",
            evaluateReconnectWifi(false, 500, 1000, &dur) == WIFI_STILL_LOST);
    }

    // --- Reconnected after loss -> RECONNECTED ---
    {
        WifiEvent ev = evaluateReconnectWifi(true, 2000, 15000, &dur);
        RUN_TEST("reconnect: reconnected -> RECONNECTED", ev == WIFI_RECONNECTED);
        RUN_TEST("reconnect: duration 13s", dur == 13);
    }

    // --- Reconnected with immediate check (0s duration) ---
    {
        WifiEvent ev = evaluateReconnectWifi(true, 10000, 10000, &dur);
        RUN_TEST("reconnect: 0s duration", ev == WIFI_RECONNECTED);
        RUN_TEST("reconnect: duration 0", dur == 0);
    }

    // --- Null duration pointer ---
    {
        RUN_TEST("reconnect: null duration -> RECONNECTED",
            evaluateReconnectWifi(true, 1000, 5000, NULL) == WIFI_RECONNECTED);
    }

    puts("\n=== connectToWiFi Tests ===\n");

    unsigned long connectedAt = 0;
    bool wasEverConnected = false;
    unsigned long disconnectedSince = 0;

    // --- Immediate connect ---
    {
        connectedAt = 0; wasEverConnected = false; disconnectedSince = 0;
        WiFi.resetCounters();
        WiFi.setStatus(WL_CONNECTED);
        mock_millis = 5000;
        bool ok = connectToWiFi("test", "pass", connectedAt, wasEverConnected, disconnectedSince);
        RUN_TEST("immediate connect returns true", ok == true);
        RUN_TEST("immediate: connectedAt set", connectedAt == 5000);
        RUN_TEST("immediate: wasEverConnected", wasEverConnected == true);
        RUN_TEST("immediate: disconnectedSince cleared", disconnectedSince == 0);
    }

    // --- Connect after 3 calls (2 failures then success) ---
    {
        connectedAt = 0; wasEverConnected = false; disconnectedSince = 0;
        WiFi.resetCounters();
        WiFi.setStatus(WL_DISCONNECTED);
        WiFi.setConnectAfterCalls(3);
        mock_millis = 50000;
        bool ok = connectToWiFi("test", "pass", connectedAt, wasEverConnected, disconnectedSince);
        RUN_TEST("connect after 3 attempts returns true", ok == true);
        RUN_TEST("connect after 3: wasEverConnected", wasEverConnected == true);
        RUN_TEST("connect after 3: disconnectedSince cleared", disconnectedSince == 0);
    }

    // --- Never connects ---
    {
        connectedAt = 0; wasEverConnected = false; disconnectedSince = 0;
        WiFi.resetCounters();
        WiFi.setStatus(WL_DISCONNECTED);
        mock_millis = 50000;
        bool ok = connectToWiFi("test", "pass", connectedAt, wasEverConnected, disconnectedSince);
        RUN_TEST("never connect returns false", ok == false);
        RUN_TEST("never: wasEverConnected still false", wasEverConnected == false);
        RUN_TEST("never: disconnectedSince set", disconnectedSince == 50000);
    }

    // --- Already wasEverConnected, disconnectSince already set, connect fails ---
    {
        connectedAt = 0; wasEverConnected = true; disconnectedSince = 1000;
        WiFi.resetCounters();
        WiFi.setStatus(WL_DISCONNECTED);
        mock_millis = 50000;
        bool ok = connectToWiFi("test", "pass", connectedAt, wasEverConnected, disconnectedSince);
        RUN_TEST("still fails: returns false", ok == false);
        RUN_TEST("still fails: disconnectedSince unchanged (already set)", disconnectedSince == 1000);
    }

    puts("\n---\nAll wifi tests passed!");
    return 0;
}
