#include "update_utils.h"
#include "eeprom_map.h"
#include "EEPROM.h"
#include "ESP8266WiFi.h"
#include "ESP8266HTTPClient.h"
#include "Updater.h"
#include "Arduino.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

#define RUN_TEST(name, expr) do {                                    \
    printf("  TEST: %s ... ", name);                                 \
    if (!(expr)) { printf("FAIL\n"); return 1; }                     \
    printf("PASS\n");                                                \
} while(0)

int main() {
    puts("\n=== parseGitHubRelease Tests ===\n");

    String latest, url;

    // --- Newer version found ---
    {
        String body = "{\"tag_name\":\"v1.2.3\",\"assets\":[{\"browser_download_url\":\"http://example.com/fw.bin\"}]}";
        RUN_TEST("newer version found", parseGitHubRelease(body, latest, url, "v1.0.0") == true);
        RUN_TEST("newer: latestTag set", latest == "v1.2.3");
        RUN_TEST("newer: downloadUrl set", url == "http://example.com/fw.bin");
    }

    // --- Same version ---
    {
        String body = "{\"tag_name\":\"v1.0.0\",\"assets\":[]}";
        RUN_TEST("same version", parseGitHubRelease(body, latest, url, "v1.0.0") == false);
    }

    // --- Older version ---
    {
        String body = "{\"tag_name\":\"v0.9.0\",\"assets\":[]}";
        RUN_TEST("older version", parseGitHubRelease(body, latest, url, "v1.0.0") == false);
    }

    // --- Invalid JSON ---
    {
        String body = "not json at all";
        RUN_TEST("invalid JSON", parseGitHubRelease(body, latest, url, "v1.0.0") == false);
    }

    // --- Missing tag_name ---
    {
        String body = "{\"other\":\"value\"}";
        RUN_TEST("missing tag_name", parseGitHubRelease(body, latest, url, "v1.0.0") == false);
    }

    // --- Empty tag_name ---
    {
        String body = "{\"tag_name\":\"\"}";
        RUN_TEST("empty tag_name", parseGitHubRelease(body, latest, url, "v1.0.0") == false);
    }

    // --- No assets (no download URL) but newer version ---
    {
        String body = "{\"tag_name\":\"v2.0.0\"}";
        RUN_TEST("no assets but newer", parseGitHubRelease(body, latest, url, "v1.0.0") == true);
        RUN_TEST("no assets: tag set", latest == "v2.0.0");
    }

    // --- Version without v prefix ---
    {
        String body = "{\"tag_name\":\"2.0.0\"}";
        RUN_TEST("no v prefix", parseGitHubRelease(body, latest, url, "1.0.0") == true);
        RUN_TEST("no v prefix tag", latest == "2.0.0");
    }

    // --- Truncated JSON ---
    {
        String body = "{\"tag_name\":\"v1.0.0\"";
        RUN_TEST("truncated JSON", parseGitHubRelease(body, latest, url, "v0.0.9") == false);
    }

    // --- Multiple assets, picks first ---
    {
        String body = "{\"tag_name\":\"v1.5.0\",\"assets\":[{\"browser_download_url\":\"http://first.bin\"},{\"browser_download_url\":\"http://second.bin\"}]}";
        RUN_TEST("multiple assets picks first", parseGitHubRelease(body, latest, url, "v1.0.0") == true);
        RUN_TEST("first asset url", url == "http://first.bin");
    }

    puts("\n=== evaluateUpdateBoot Tests ===\n");

    // --- No pending update ---
    {
        RUN_TEST("no pending, attempts=0 -> CLEAR", evaluateUpdateBoot(0, 0, 3) == UPDATE_BOOT_CLEAR);
        RUN_TEST("no pending, attempts=5 -> CLEAR", evaluateUpdateBoot(0, 5, 3) == UPDATE_BOOT_CLEAR);
    }

    // --- Pending, attempts within limit ---
    {
        RUN_TEST("pending 1, attempts=0, max=2 -> RETRY", evaluateUpdateBoot(1, 0, 2) == UPDATE_BOOT_RETRY);
        RUN_TEST("pending 1, attempts=1, max=2 -> RETRY", evaluateUpdateBoot(1, 1, 2) == UPDATE_BOOT_RETRY);
    }

    // --- Pending, attempts at or above limit ---
    {
        RUN_TEST("pending 1, attempts=2, max=2 -> DISABLE", evaluateUpdateBoot(1, 2, 2) == UPDATE_BOOT_DISABLE);
        RUN_TEST("pending 1, attempts=3, max=2 -> DISABLE", evaluateUpdateBoot(1, 3, 2) == UPDATE_BOOT_DISABLE);
        RUN_TEST("pending 1, attempts=0, max=0 -> DISABLE", evaluateUpdateBoot(1, 0, 0) == UPDATE_BOOT_DISABLE);
    }

    // --- Different max values ---
    {
        RUN_TEST("pending 1, attempts=4, max=5 -> RETRY", evaluateUpdateBoot(1, 4, 5) == UPDATE_BOOT_RETRY);
        RUN_TEST("pending 1, attempts=5, max=5 -> DISABLE", evaluateUpdateBoot(1, 5, 5) == UPDATE_BOOT_DISABLE);
    }

    puts("\n=== handleUpdateBootEEPROM Tests ===\n");

    // --- CLEAR: no pending update ---
    {
        EEPROM.begin(127);
        for (int i = 0; i < 127; i++) EEPROM.write(i, 0);
        EEPROM.write(UPDATE_PENDING_ADDR, 0);
        EEPROM.write(UPDATE_ATTEMPTS_ADDR, 5);

        uint8_t attempts = 99;
        UpdateBootAction a = handleUpdateBootEEPROM(2, attempts);
        RUN_TEST("no pending -> CLEAR", a == UPDATE_BOOT_CLEAR);
        RUN_TEST("CLEAR: attempts loaded", attempts == 5);
        RUN_TEST("CLEAR: attempts reset in EEPROM", EEPROM.read(UPDATE_ATTEMPTS_ADDR) == 0);
    }

    // --- DISABLE: pending, max attempts reached ---
    {
        EEPROM.begin(127);
        for (int i = 0; i < 127; i++) EEPROM.write(i, 0);
        EEPROM.write(UPDATE_PENDING_ADDR, 1);
        EEPROM.write(UPDATE_ATTEMPTS_ADDR, 2);
        EEPROM.write(AUTO_UPDATE_ADDR, 1);

        uint8_t attempts = 99;
        UpdateBootAction a = handleUpdateBootEEPROM(2, attempts);
        RUN_TEST("pending + max attempts -> DISABLE", a == UPDATE_BOOT_DISABLE);
        RUN_TEST("DISABLE: autoUpdate cleared", EEPROM.read(AUTO_UPDATE_ADDR) == 0);
        RUN_TEST("DISABLE: pending cleared", EEPROM.read(UPDATE_PENDING_ADDR) == 0);
        RUN_TEST("DISABLE: attempts cleared", EEPROM.read(UPDATE_ATTEMPTS_ADDR) == 0);
    }

    // --- RETRY: pending, attempts < max ---
    {
        EEPROM.begin(127);
        for (int i = 0; i < 127; i++) EEPROM.write(i, 0);
        EEPROM.write(UPDATE_PENDING_ADDR, 1);
        EEPROM.write(UPDATE_ATTEMPTS_ADDR, 1);

        uint8_t attempts = 99;
        UpdateBootAction a = handleUpdateBootEEPROM(3, attempts);
        RUN_TEST("pending + under max -> RETRY", a == UPDATE_BOOT_RETRY);
        RUN_TEST("RETRY: attempts loaded", attempts == 1);
        RUN_TEST("RETRY: attempts incremented in EEPROM", EEPROM.read(UPDATE_ATTEMPTS_ADDR) == 2);
        RUN_TEST("RETRY: pending preserved", EEPROM.read(UPDATE_PENDING_ADDR) == 1);
    }

    // --- RETRY at boundary: pending, attempts = max - 1 ---
    {
        EEPROM.begin(127);
        for (int i = 0; i < 127; i++) EEPROM.write(i, 0);
        EEPROM.write(UPDATE_PENDING_ADDR, 1);
        EEPROM.write(UPDATE_ATTEMPTS_ADDR, 4);

        uint8_t attempts = 99;
        UpdateBootAction a = handleUpdateBootEEPROM(5, attempts);
        RUN_TEST("at boundary (4/5) -> RETRY", a == UPDATE_BOOT_RETRY);
        RUN_TEST("boundary: attempts incremented", EEPROM.read(UPDATE_ATTEMPTS_ADDR) == 5);
    }

    // --- DISABLE at boundary: pending, attempts = max ---
    {
        EEPROM.begin(127);
        for (int i = 0; i < 127; i++) EEPROM.write(i, 0);
        EEPROM.write(UPDATE_PENDING_ADDR, 1);
        EEPROM.write(UPDATE_ATTEMPTS_ADDR, 5);
        EEPROM.write(AUTO_UPDATE_ADDR, 1);

        uint8_t attempts = 99;
        UpdateBootAction a = handleUpdateBootEEPROM(5, attempts);
        RUN_TEST("at boundary (5/5) -> DISABLE", a == UPDATE_BOOT_DISABLE);
        RUN_TEST("boundary: autoUpdate cleared", EEPROM.read(AUTO_UPDATE_ADDR) == 0);
    }

    puts("\n=== checkForUpdates Tests ===\n");

    // --- WiFi not connected ---
    {
        WiFiClient wc;
        WiFi.resetCounters();
        WiFi.setStatus(WL_DISCONNECTED);
        String tag, url;
        bool ok = checkForUpdates(tag, url, "v1.0.0", wc);
        RUN_TEST("not connected: returns false", ok == false);
        RUN_TEST("not connected: tag empty", tag == "");
    }

    // --- WiFi connected, HTTP 200 with update ---
    {
        WiFiClient wc;
        WiFi.resetCounters();
        WiFi.setStatus(WL_CONNECTED);
        HTTPClient::setHttpCode(200);
        HTTPClient::setPayload("{\"tag_name\":\"v2.0.0\",\"assets\":[{\"browser_download_url\":\"http://example.com/fw.bin\"}]}");
        String tag, url;
        bool ok = checkForUpdates(tag, url, "v1.0.0", wc);
        RUN_TEST("HTTP 200 with update: returns true", ok == true);
        RUN_TEST("HTTP 200 with update: tag set", tag == "v2.0.0");
        RUN_TEST("HTTP 200 with update: url set", url == "http://example.com/fw.bin");
    }

    // --- WiFi connected, HTTP 200 no update (same version) ---
    {
        WiFiClient wc;
        WiFi.resetCounters();
        WiFi.setStatus(WL_CONNECTED);
        HTTPClient::setHttpCode(200);
        HTTPClient::setPayload("{\"tag_name\":\"v1.0.0\",\"assets\":[]}");
        String tag, url;
        bool ok = checkForUpdates(tag, url, "v1.0.0", wc);
        RUN_TEST("HTTP 200 same version: returns false", ok == false);
    }

    // --- WiFi connected, HTTP error (404) ---
    {
        WiFiClient wc;
        WiFi.resetCounters();
        WiFi.setStatus(WL_CONNECTED);
        HTTPClient::setHttpCode(404);
        HTTPClient::setPayload("Not Found");
        String tag = "unchanged", url = "unchanged";
        bool ok = checkForUpdates(tag, url, "v1.0.0", wc);
        RUN_TEST("HTTP 404: returns false", ok == false);
        RUN_TEST("HTTP 404: tag unchanged", tag == "unchanged");
    }

    // --- WiFi connected, HTTP 200 with invalid JSON ---
    {
        WiFiClient wc;
        WiFi.resetCounters();
        WiFi.setStatus(WL_CONNECTED);
        HTTPClient::setHttpCode(200);
        HTTPClient::setPayload("not json");
        String tag, url;
        bool ok = checkForUpdates(tag, url, "v1.0.0", wc);
        RUN_TEST("HTTP 200 bad json: returns false", ok == false);
    }

    // --- WiFi connected, HTTP code 0 (connection failure) ---
    {
        WiFiClient wc;
        WiFi.resetCounters();
        WiFi.setStatus(WL_CONNECTED);
        HTTPClient::setHttpCode(0);
        HTTPClient::setPayload("");
        String tag, url;
        bool ok = checkForUpdates(tag, url, "v1.0.0", wc);
        RUN_TEST("HTTP 0: returns false", ok == false);
    }

    puts("\n=== flashOTAUpdate Tests ===\n");

    // --- HTTP error (404) ---
    {
        HTTPClient http;
        WiFiClient stream;
        UpdaterClass updater;
        HTTPClient::setConnected(true);
        bool ok = flashOTAUpdate(http, stream, updater, 404, 1024);
        RUN_TEST("HTTP 404: returns false", ok == false);
        RUN_TEST("HTTP 404: nothing written to updater", updater.written() == 0);
    }

    // --- Invalid content length (0) ---
    {
        HTTPClient http;
        WiFiClient stream;
        UpdaterClass updater;
        HTTPClient::setConnected(true);
        bool ok = flashOTAUpdate(http, stream, updater, 200, 0);
        RUN_TEST("content length 0: returns false", ok == false);
    }

    // --- Invalid content length (-1) ---
    {
        HTTPClient http;
        WiFiClient stream;
        UpdaterClass updater;
        HTTPClient::setConnected(true);
        bool ok = flashOTAUpdate(http, stream, updater, 200, -1);
        RUN_TEST("content length -1: returns false", ok == false);
    }

    // --- Update.begin fails ---
    {
        HTTPClient http;
        WiFiClient stream;
        UpdaterClass updater;
        updater.setBeginResult(false);
        HTTPClient::setConnected(true);
        bool ok = flashOTAUpdate(http, stream, updater, 200, 1024);
        RUN_TEST("Update.begin fails: returns false", ok == false);
        RUN_TEST("Update.begin fails: nothing written", updater.written() == 0);
    }

    // --- Successful OTA download and flash ---
    {
        HTTPClient http;
        WiFiClient stream;
        stream.setStreamBytes(2048);
        UpdaterClass updater;
        HTTPClient::setConnected(true);
        bool ok = flashOTAUpdate(http, stream, updater, 200, 2048);
        RUN_TEST("successful OTA: returns true", ok == true);
        RUN_TEST("successful OTA: all bytes written", updater.written() == 2048);
        RUN_TEST("successful OTA: write called 4 times (512*4=2048)", updater.writeCount() == 4);
    }

    // --- Update.end fails after successful download ---
    {
        HTTPClient http;
        WiFiClient stream;
        stream.setStreamBytes(1024);
        UpdaterClass updater;
        updater.setEndResult(false);
        HTTPClient::setConnected(true);
        bool ok = flashOTAUpdate(http, stream, updater, 200, 1024);
        RUN_TEST("Update.end fails: returns false", ok == false);
        RUN_TEST("Update.end fails: bytes still written", updater.written() == 1024);
    }

    puts("\n---\nAll update tests passed!");
    return 0;
}
