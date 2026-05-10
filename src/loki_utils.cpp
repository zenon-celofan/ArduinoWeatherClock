#include "loki_utils.h"
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
