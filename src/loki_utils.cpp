#include "loki_utils.h"
#include "eeprom_config.h"
#include "eeprom_map.h"
#include "eeprom_utils.h"
#include "url_utils.h"
#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <sys/time.h>
#include <ArduinoJson.h>
#include <string>
#include <cstring>

static const unsigned long long TIMESTAMP_MIN_VALID_SEC = 1735689600ULL; // 2025-01-01

bool isTimestampValid(unsigned long long epochSec) {
    return epochSec >= TIMESTAMP_MIN_VALID_SEC;
}

String buildLokiPayload(const String &deviceName, const String &category, const String &logMessage, unsigned long long epochNanoseconds) {
    JsonDocument doc;
    doc["streams"][0]["stream"]["device"] = deviceName.c_str();
    doc["streams"][0]["stream"]["level"] = "info";
    doc["streams"][0]["stream"]["category"] = category.c_str();
    doc["streams"][0]["values"][0][0] = std::to_string(epochNanoseconds);
    doc["streams"][0]["values"][0][1] = logMessage.c_str();

    std::string json;
    serializeJson(doc, json);
    return String(json.c_str());
}

String buildLokiUrlFromEEPROM() {
    String ip1 = readStringFromEEPROM(LOKI_IP1_ADDR, 3);
    String ip2 = readStringFromEEPROM(LOKI_IP2_ADDR, 3);
    String ip3 = readStringFromEEPROM(LOKI_IP3_ADDR, 3);
    String ip4 = readStringFromEEPROM(LOKI_IP4_ADDR, 3);
    String port = readStringFromEEPROM(LOKI_PORT_ADDR, 5);
    String ip = ip1 + "." + ip2 + "." + ip3 + "." + ip4;
    return buildLokiUrl(ip, port);
}

bool sendLokiLog(const String &lokiURL, const String &deviceName,
                 const String &category, const String &logMessage) {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    if (!isTimestampValid((unsigned long long)tv.tv_sec)) {
        return false;
    }

    unsigned long long epochNanoseconds =
        (unsigned long long)(tv.tv_sec) * 1000000000ULL +
        (unsigned long long)(tv.tv_usec) * 1000ULL;

    String jsonPayload = buildLokiPayload(deviceName, category, logMessage, epochNanoseconds);

    HTTPClient http;
    WiFiClient client;
    http.begin(client, lokiURL);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(jsonPayload);
    bool ok = (httpResponseCode == 204 || httpResponseCode == 200);
    if (ok) {
        Serial.println("Loki OK [HTTP " + String(httpResponseCode) + "]: " + logMessage);
    } else {
        Serial.println("Loki FAILED [HTTP " + String(httpResponseCode) + "]: " + logMessage);
    }
    http.end();
    return ok;
}

bool sendLokiIfEnabled(const String &lokiURL, const String &deviceName,
                       const String &category, const String &logMessage) {
    if (!loadLokiEnabled()) return false;
    if (WiFi.status() != WL_CONNECTED) return false;
    if (lokiURL.length() == 0) return false;
    return sendLokiLog(lokiURL, deviceName, category, logMessage);
}
