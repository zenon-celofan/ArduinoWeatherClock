#include "weather_utils.h"
#include "url_utils.h"
#include "eeprom_config.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
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

bool fetchTemperatureAndTimezone(const String &latitude, const String &longitude,
                                 float &temperature, long &gmtOffsetSec,
                                 int &daylightOffsetSec) {
    if (WiFi.status() != WL_CONNECTED) return false;

    HTTPClient http;
    WiFiClient client;
    String url = buildOpenMeteoUrl(latitude, longitude);
    Serial.print("Request URL: ");
    Serial.println(url);
    http.begin(client, url);
    int httpCode = http.GET();
    if (httpCode > 0) {
        String payload = http.getString();
        WeatherData data;
        if (!parseOpenMeteoResponse(payload, data)) {
            http.end();
            return false;
        }
        temperature = data.temperature;
        gmtOffsetSec = data.utcOffsetSec;
        daylightOffsetSec = data.daylightOffsetSec;
        http.end();
        return true;
    }
    http.end();
    return false;
}

bool updateLocalWeatherData(float &temperature, long &gmtOffsetSec,
                            int &daylightOffsetSec) {
    String latitude, longitude;
    loadLocationData(latitude, longitude);
    return fetchTemperatureAndTimezone(latitude, longitude, temperature,
                                       gmtOffsetSec, daylightOffsetSec);
}
