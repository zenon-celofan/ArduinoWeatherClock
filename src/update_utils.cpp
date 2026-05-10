#include "update_utils.h"
#include "semver_compare.h"
#include "eeprom_map.h"
#include <EEPROM.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Updater.h>
#include <ArduinoJson.h>
#include <cstring>

UpdateBootAction evaluateUpdateBoot(uint8_t pending, uint8_t attempts, uint8_t maxAttempts) {
    if (pending == 0) return UPDATE_BOOT_CLEAR;
    if (attempts >= maxAttempts) return UPDATE_BOOT_DISABLE;
    return UPDATE_BOOT_RETRY;
}

bool parseGitHubRelease(const String &jsonBody, String &latestTag, String &downloadUrl, const String &currentVersion) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, jsonBody.c_str());
    if (err) return false;

    const char *tag = doc["tag_name"];
    if (!tag || strlen(tag) == 0) return false;
    latestTag = tag;

    String foundUrl;
    if (doc["assets"].size() > 0) {
        const char *url = doc["assets"][0]["browser_download_url"];
        if (url) foundUrl = url;
    }
    if (foundUrl.length() == 0) return false;
    downloadUrl = foundUrl;

    return semverCompare(latestTag, currentVersion) > 0;
}

UpdateBootAction handleUpdateBootEEPROM(uint8_t maxAttempts, uint8_t &outAttempts) {
    uint8_t pending = EEPROM.read(UPDATE_PENDING_ADDR);
    outAttempts = EEPROM.read(UPDATE_ATTEMPTS_ADDR);

    UpdateBootAction action = evaluateUpdateBoot(pending, outAttempts, maxAttempts);

    switch (action) {
        case UPDATE_BOOT_CLEAR:
            EEPROM.write(UPDATE_ATTEMPTS_ADDR, 0);
            EEPROM.commit();
            break;

        case UPDATE_BOOT_DISABLE:
            Serial.printf("Update pending detected (attempts: %d), max reached, disabling auto-update\n", outAttempts);
            EEPROM.write(AUTO_UPDATE_ADDR, 0);
            EEPROM.write(UPDATE_PENDING_ADDR, 0);
            EEPROM.write(UPDATE_ATTEMPTS_ADDR, 0);
            EEPROM.commit();
            break;

        case UPDATE_BOOT_RETRY:
            Serial.printf("Update pending detected (attempts: %d), running sanity timer (attempt %d/%d)...\n",
                outAttempts, outAttempts + 1, maxAttempts);
            EEPROM.write(UPDATE_ATTEMPTS_ADDR, outAttempts + 1);
            EEPROM.commit();
            break;
    }
    return action;
}

bool checkForUpdates(String &latestTag, String &downloadUrl,
                     const String &firmwareVersion, WiFiClient &client) {
    if (WiFi.status() != WL_CONNECTED) return false;

    HTTPClient http;
    http.begin(client, "https://api.github.com/repos/zenon-celofan/ArduinoWeatherClock/releases/latest");
    http.addHeader("User-Agent", "ESP8266-Weather-Clock");
    http.addHeader("Accept", "application/vnd.github+json");

    int httpCode = http.GET();
    if (httpCode <= 0) {
        http.end();
        return false;
    }

    String body = http.getString();
    http.end();

    return parseGitHubRelease(body, latestTag, downloadUrl, firmwareVersion);
}

bool flashOTAUpdate(HTTPClient &http, WiFiClient &stream,
                    UpdaterClass &updater, int httpCode, int contentLength) {
    Serial.printf("Download HTTP code: %d\n", httpCode);
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("Download failed: %d\n", httpCode);
        http.end();
        return false;
    }

    Serial.printf("Firmware size: %d bytes\n", contentLength);
    if (contentLength <= 0) {
        Serial.println("Invalid content length");
        http.end();
        return false;
    }

    if (!updater.begin(contentLength, U_FLASH)) {
        Serial.printf("Update.begin failed: %s\n", updater.getErrorString());
        http.end();
        return false;
    }

    Serial.println("Flashing firmware...");
    uint8_t buff[1024] = { 0 };
    size_t totalWritten = 0;

    unsigned long start = millis();
    while (http.connected() && totalWritten < (size_t)contentLength) {
        if (millis() - start > 60000) {
            Serial.println("Download timeout");
            break;
        }
        size_t avail = stream.available();
        if (avail) {
            size_t toRead = (avail > sizeof(buff)) ? sizeof(buff) : avail;
            size_t read = stream.readBytes(buff, toRead);
            updater.write(buff, read);
            totalWritten += read;
            start = millis();
            if (totalWritten % 50000 < read) {
                Serial.printf("  Progress: %d/%d bytes\n", totalWritten, contentLength);
            }
        }
        yield();
    }
    http.end();

    if (updater.end(true)) {
        Serial.printf("Flash complete: %d bytes written\n", totalWritten);
        return true;
    } else {
        Serial.printf("Flash failed: %s\n", updater.getErrorString());
        return false;
    }
}
