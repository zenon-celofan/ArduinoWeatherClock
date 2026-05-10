#include "weather_utils.h"
#include <ArduinoJson.h>
#include <cstring>

bool parseOpenMeteoResponse(const String &jsonBody, WeatherData &data) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, jsonBody.c_str());
    if (err) return false;

    data.temperature = doc["current_weather"]["temperature"];
    data.utcOffsetSec = doc["utc_offset_seconds"];

    const char *tzAbbr = doc["timezone_abbreviation"];
    data.daylightOffsetSec = (tzAbbr && strcmp(tzAbbr, "DST") == 0) ? 3600 : 0;

    return true;
}
