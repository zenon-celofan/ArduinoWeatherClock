#include "loki_utils.h"
#include "eeprom_config.h"
#include "eeprom_map.h"
#include "eeprom_utils.h"
#include "EEPROM.h"
#include "ESP8266HTTPClient.h"
#include "ESP8266WiFi.h"
#include "Arduino.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sys/time.h>

#define RUN_TEST(name, expr) do {                                    \
    printf("  TEST: %s ... ", name);                                 \
    if (!(expr)) { printf("FAIL\n"); return 1; }                     \
    printf("PASS\n");                                                \
} while(0)

int main() {
    puts("\n=== buildLokiPayload Tests ===\n");

    String result;

    // --- Basic payload structure ---
    {
        result = buildLokiPayload("sensor-01", "test", "hello", 1234567890ULL);
        const char *r = result.c_str();
        RUN_TEST("contains device", strstr(r, "\"device\":\"sensor-01\"") != NULL);
        RUN_TEST("contains category", strstr(r, "\"category\":\"test\"") != NULL);
        RUN_TEST("contains level info", strstr(r, "\"level\":\"info\"") != NULL);
        RUN_TEST("contains message", strstr(r, "\"hello\"") != NULL);
        RUN_TEST("contains timestamp", strstr(r, "1234567890") != NULL);
        RUN_TEST("starts with {", r[0] == '{');
        RUN_TEST("ends with }", r[result.length() - 1] == '}');
    }

    // --- Device name with spaces and special chars ---
    {
        result = buildLokiPayload("my device-2", "system", "test msg", 0ULL);
        RUN_TEST("device with spaces", strstr(result.c_str(), "\"device\":\"my device-2\"") != NULL);
    }

    // --- Category with underscore ---
    {
        result = buildLokiPayload("dev", "wifi_status", "msg", 0ULL);
        RUN_TEST("category underscore", strstr(result.c_str(), "\"category\":\"wifi_status\"") != NULL);
    }

    // --- Message with special characters ---
    {
        result = buildLokiPayload("dev", "cat", "line1\nline2\t\"quoted\"", 0ULL);
        RUN_TEST("message special chars", strstr(result.c_str(), "line1") != NULL);
        RUN_TEST("tab in message", strstr(result.c_str(), "line2") != NULL);
        RUN_TEST("quotes escaped", strstr(result.c_str(), "\\\"quoted\\\"") != NULL);
    }

    // --- Large timestamp ---
    {
        result = buildLokiPayload("dev", "cat", "msg", 1700000000000000000ULL);
        RUN_TEST("large timestamp", strstr(result.c_str(), "1700000000000000000") != NULL);
    }

    // --- isTimestampValid tests ---
    {
        RUN_TEST("timestamp before 2025 invalid", isTimestampValid(1000000) == false);
        RUN_TEST("timestamp at boundary valid", isTimestampValid(1735689600ULL) == true);
        RUN_TEST("timestamp after boundary valid", isTimestampValid(1800000000ULL) == true);
        RUN_TEST("timestamp zero invalid", isTimestampValid(0) == false);
        RUN_TEST("timestamp 2024 invalid", isTimestampValid(1700000000ULL) == false);
    }

    // --- buildLokiUrlFromEEPROM tests ---
    puts("\n=== buildLokiUrlFromEEPROM Tests ===\n");
    {
        EEPROM.begin(127);
        for (int i = 0; i < 127; i++) EEPROM.write(i, 0);
        EEPROM.write(LOKI_IP1_ADDR, '1');
        EEPROM.write(LOKI_IP1_ADDR + 1, '9');
        EEPROM.write(LOKI_IP1_ADDR + 2, '2');
        EEPROM.write(LOKI_IP2_ADDR, '1');
        EEPROM.write(LOKI_IP2_ADDR + 1, '6');
        EEPROM.write(LOKI_IP2_ADDR + 2, '8');
        EEPROM.write(LOKI_IP3_ADDR, '1');
        EEPROM.write(LOKI_IP4_ADDR, '1');
        EEPROM.write(LOKI_IP4_ADDR + 1, '0');
        EEPROM.write(LOKI_PORT_ADDR + 0, '3');
        EEPROM.write(LOKI_PORT_ADDR + 1, '1');
        EEPROM.write(LOKI_PORT_ADDR + 2, '0');
        EEPROM.write(LOKI_PORT_ADDR + 3, '0');

        String url = buildLokiUrlFromEEPROM();
        RUN_TEST("URL starts with http://", strncmp(url.c_str(), "http://", 7) == 0);
        RUN_TEST("URL contains IP 192.168.1.10", strstr(url.c_str(), "192.168.1.10") != NULL);
        RUN_TEST("URL contains port 3100", strstr(url.c_str(), "3100") != NULL);
        RUN_TEST("URL ends with /loki/api/v1/push", strstr(url.c_str(), "/loki/api/v1/push") != NULL);
    }

    // --- Empty EEPROM returns empty URL ---
    {
        EEPROM.begin(127);
        for (int i = 0; i < 127; i++) EEPROM.write(i, 0);
        String url = buildLokiUrlFromEEPROM();
        RUN_TEST("empty EEPROM returns http://..", strncmp(url.c_str(), "http://", 7) == 0);
    }

    puts("\n=== sendLokiLog Tests ===\n");

    // --- HTTP 204 success ---
    {
        HTTPClient::setHttpCode(204);
        bool ok = sendLokiLog("http://loki:3100", "dev", "test", "hello");
        RUN_TEST("HTTP 204 returns true", ok == true);
    }

    // --- HTTP 200 success ---
    {
        HTTPClient::setHttpCode(200);
        bool ok = sendLokiLog("http://loki:3100", "dev", "test", "hello");
        RUN_TEST("HTTP 200 returns true", ok == true);
    }

    // --- HTTP 400 failure ---
    {
        HTTPClient::setHttpCode(400);
        bool ok = sendLokiLog("http://loki:3100", "dev", "test", "hello");
        RUN_TEST("HTTP 400 returns false", ok == false);
    }

    // --- HTTP 500 failure ---
    {
        HTTPClient::setHttpCode(500);
        bool ok = sendLokiLog("http://loki:3100", "dev", "test", "hello");
        RUN_TEST("HTTP 500 returns false", ok == false);
    }

    // --- HTTP -1 connection error ---
    {
        HTTPClient::setHttpCode(-1);
        bool ok = sendLokiLog("http://loki:3100", "dev", "test", "hello");
        RUN_TEST("HTTP -1 returns false", ok == false);
    }

    // --- sendLokiIfEnabled tests ---
    puts("\n=== sendLokiIfEnabled Tests ===\n");

    // --- Loki disabled (EEPROM LOKI_ENABLED_ADDR = 0) ---
    {
        EEPROM.reset();
        EEPROM.begin(127);
        WiFi.setStatus(WL_CONNECTED);
        HTTPClient::resetPostCallCount();
        HTTPClient::setHttpCode(204);

        bool ok = sendLokiIfEnabled("http://loki:3100", "dev", "test", "msg");
        RUN_TEST("loki disabled returns false", ok == false);
        RUN_TEST("loki disabled: no POST call", HTTPClient::postCallCount() == 0);
    }

    // --- Loki enabled, WiFi not connected ---
    {
        EEPROM.reset();
        EEPROM.begin(127);
        EEPROM.write(LOKI_ENABLED_ADDR, 1);
        WiFi.setStatus(WL_DISCONNECTED);
        HTTPClient::resetPostCallCount();
        HTTPClient::setHttpCode(204);

        bool ok = sendLokiIfEnabled("http://loki:3100", "dev", "test", "msg");
        RUN_TEST("wifi not connected returns false", ok == false);
        RUN_TEST("wifi not connected: no POST call", HTTPClient::postCallCount() == 0);
    }

    // --- Loki enabled, WiFi connected, empty URL ---
    {
        EEPROM.reset();
        EEPROM.begin(127);
        EEPROM.write(LOKI_ENABLED_ADDR, 1);
        WiFi.setStatus(WL_CONNECTED);
        HTTPClient::resetPostCallCount();
        HTTPClient::setHttpCode(204);

        bool ok = sendLokiIfEnabled("", "dev", "test", "msg");
        RUN_TEST("empty URL returns false", ok == false);
        RUN_TEST("empty URL: no POST call", HTTPClient::postCallCount() == 0);
    }

    // --- All conditions met, HTTP 204 ---
    {
        EEPROM.reset();
        EEPROM.begin(127);
        EEPROM.write(LOKI_ENABLED_ADDR, 1);
        WiFi.setStatus(WL_CONNECTED);
        HTTPClient::resetPostCallCount();
        HTTPClient::setHttpCode(204);

        bool ok = sendLokiIfEnabled("http://loki:3100", "dev", "test", "msg");
        RUN_TEST("all ok HTTP 204 returns true", ok == true);
        RUN_TEST("all ok: POST was called", HTTPClient::postCallCount() >= 1);
    }

    // --- All conditions met, HTTP 400 ---
    {
        EEPROM.reset();
        EEPROM.begin(127);
        EEPROM.write(LOKI_ENABLED_ADDR, 1);
        WiFi.setStatus(WL_CONNECTED);
        HTTPClient::resetPostCallCount();
        HTTPClient::setHttpCode(400);

        bool ok = sendLokiIfEnabled("http://loki:3100", "dev", "test", "msg");
        RUN_TEST("all ok HTTP 400 returns false", ok == false);
        RUN_TEST("all ok HTTP 400: POST was called", HTTPClient::postCallCount() >= 1);
    }

    // --- All conditions met, HTTP 200 ---
    {
        EEPROM.reset();
        EEPROM.begin(127);
        EEPROM.write(LOKI_ENABLED_ADDR, 1);
        WiFi.setStatus(WL_CONNECTED);
        HTTPClient::resetPostCallCount();
        HTTPClient::setHttpCode(200);

        bool ok = sendLokiIfEnabled("http://loki:3100", "dev", "test", "msg");
        RUN_TEST("all ok HTTP 200 returns true", ok == true);
        RUN_TEST("all ok HTTP 200: POST was called", HTTPClient::postCallCount() >= 1);
    }

    puts("\n---\nAll loki tests passed!");
    return 0;
}
