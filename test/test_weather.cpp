#include "weather_utils.h"
#include "eeprom_utils.h"
#include "eeprom_map.h"
#include "EEPROM.h"
#include "ESP8266WiFi.h"
#include "ESP8266HTTPClient.h"
#include "Arduino.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>

#define RUN_TEST(name, expr) do {                                    \
    printf("  TEST: %s ... ", name);                                 \
    if (!(expr)) { printf("FAIL\n"); return 1; }                     \
    printf("PASS\n");                                                \
} while(0)

static bool float_eq(float a, float b, float eps = 0.001f) {
    return fabs(a - b) < eps;
}

int main() {
    puts("\n=== parseOpenMeteoResponse Tests ===\n");

    WeatherData data;

    // --- Normal response with non-DST timezone ---
    {
        String body = "{\"current_weather\":{\"temperature\":23.5},\"utc_offset_seconds\":3600,\"timezone_abbreviation\":\"CET\"}";
        RUN_TEST("valid response", parseOpenMeteoResponse(body, data) == true);
        RUN_TEST("temperature 23.5", float_eq(data.temperature, 23.5f));
        RUN_TEST("utc offset 3600", data.utcOffsetSec == 3600);
        RUN_TEST("daylight offset 0 (CET)", data.daylightOffsetSec == 0);
    }

    // --- DST timezone abbreviation ---
    {
        String body = "{\"current_weather\":{\"temperature\":10.0},\"utc_offset_seconds\":7200,\"timezone_abbreviation\":\"DST\"}";
        RUN_TEST("DST valid", parseOpenMeteoResponse(body, data) == true);
        RUN_TEST("DST daylight offset 3600", data.daylightOffsetSec == 3600);
    }

    // --- Negative temperature ---
    {
        String body = "{\"current_weather\":{\"temperature\":-5.2},\"utc_offset_seconds\":0,\"timezone_abbreviation\":\"UTC\"}";
        RUN_TEST("negative temp", parseOpenMeteoResponse(body, data) == true);
        RUN_TEST("temp -5.2", float_eq(data.temperature, -5.2f));
    }

    // --- Integer temperature ---
    {
        String body = "{\"current_weather\":{\"temperature\":15},\"utc_offset_seconds\":-18000,\"timezone_abbreviation\":\"EST\"}";
        RUN_TEST("int temp 15", parseOpenMeteoResponse(body, data) == true);
        RUN_TEST("int temp value", float_eq(data.temperature, 15.0f));
        RUN_TEST("negative utc offset", data.utcOffsetSec == -18000);
    }

    // --- Invalid JSON ---
    {
        String body = "not json";
        RUN_TEST("invalid JSON", parseOpenMeteoResponse(body, data) == false);
    }

    // --- Empty JSON object ---
    {
        String body = "{}";
        RUN_TEST("empty JSON", parseOpenMeteoResponse(body, data) == true);
        RUN_TEST("empty: temp defaults to 0", float_eq(data.temperature, 0.0f));
        RUN_TEST("empty: offset defaults to 0", data.utcOffsetSec == 0);
    }

    // --- Missing timezone_abbreviation ---
    {
        String body = "{\"current_weather\":{\"temperature\":20},\"utc_offset_seconds\":3600}";
        RUN_TEST("missing tz abbr", parseOpenMeteoResponse(body, data) == true);
        RUN_TEST("missing tz: daylight offset 0", data.daylightOffsetSec == 0);
    }

    puts("\n=== fetchTemperatureAndTimezone Tests ===\n");

    float temp = 0;
    long gmt = 0;
    int dst = 0;

    // --- WiFi not connected -> returns false ---
    {
        temp = 0; gmt = 0; dst = 0;
        WiFi.resetCounters();
        WiFi.setStatus(WL_DISCONNECTED);
        bool ok = fetchTemperatureAndTimezone("10.0", "20.0", temp, gmt, dst);
        RUN_TEST("not connected: returns false", ok == false);
        RUN_TEST("not connected: temp unchanged", float_eq(temp, 0.0f));
        RUN_TEST("not connected: gmt unchanged", gmt == 0);
    }

    // --- WiFi connected, HTTP 200 valid JSON ---
    {
        temp = 0; gmt = 0; dst = 0;
        WiFi.resetCounters();
        WiFi.setStatus(WL_CONNECTED);
        HTTPClient::setHttpCode(200);
        HTTPClient::setPayload("{\"current_weather\":{\"temperature\":25.5},\"utc_offset_seconds\":3600,\"timezone_abbreviation\":\"CET\"}");
        bool ok = fetchTemperatureAndTimezone("10.0", "20.0", temp, gmt, dst);
        RUN_TEST("HTTP 200 valid: returns true", ok == true);
        RUN_TEST("HTTP 200 valid: temp 25.5", float_eq(temp, 25.5f));
        RUN_TEST("HTTP 200 valid: gmt 3600", gmt == 3600);
        RUN_TEST("HTTP 200 valid: dst 0", dst == 0);
    }

    // --- WiFi connected, HTTP 200 with DST ---
    {
        temp = 0; gmt = 0; dst = 0;
        WiFi.resetCounters();
        WiFi.setStatus(WL_CONNECTED);
        HTTPClient::setHttpCode(200);
        HTTPClient::setPayload("{\"current_weather\":{\"temperature\":10.0},\"utc_offset_seconds\":7200,\"timezone_abbreviation\":\"DST\"}");
        bool ok = fetchTemperatureAndTimezone("0", "0", temp, gmt, dst);
        RUN_TEST("HTTP 200 DST: returns true", ok == true);
        RUN_TEST("HTTP 200 DST: dst 3600", dst == 3600);
    }

    // --- WiFi connected, HTTP 200 invalid JSON ---
    {
        temp = 0; gmt = 0; dst = 0;
        WiFi.resetCounters();
        WiFi.setStatus(WL_CONNECTED);
        HTTPClient::setHttpCode(200);
        HTTPClient::setPayload("not json");
        bool ok = fetchTemperatureAndTimezone("10.0", "20.0", temp, gmt, dst);
        RUN_TEST("HTTP 200 bad json: returns false", ok == false);
    }

    // --- WiFi connected, HTTP error (404) ---
    {
        temp = 42; gmt = 42; dst = 42;
        WiFi.resetCounters();
        WiFi.setStatus(WL_CONNECTED);
        HTTPClient::setHttpCode(404);
        HTTPClient::setPayload("Not Found");
        bool ok = fetchTemperatureAndTimezone("10.0", "20.0", temp, gmt, dst);
        RUN_TEST("HTTP 404: returns false", ok == false);
        RUN_TEST("HTTP 404: temp unchanged", float_eq(temp, 42.0f));
        RUN_TEST("HTTP 404: gmt unchanged", gmt == 42);
        RUN_TEST("HTTP 404: dst unchanged", dst == 42);
    }

    // --- WiFi connected, HTTP code 0 (connection error) ---
    {
        temp = 0; gmt = 0; dst = 0;
        WiFi.resetCounters();
        WiFi.setStatus(WL_CONNECTED);
        HTTPClient::setHttpCode(0);
        HTTPClient::setPayload("");
        bool ok = fetchTemperatureAndTimezone("10.0", "20.0", temp, gmt, dst);
        RUN_TEST("HTTP 0: returns false", ok == false);
    }

    puts("\n=== updateLocalWeatherData Tests ===\n");

    // --- updateLocalWeatherData with valid EEPROM + HTTP ---
    {
        EEPROM.reset();
        writeStringToEEPROM(LATITUDE_ADDR, "10.0", 15);
        writeStringToEEPROM(LONGITUDE_ADDR, "20.0", 15);
        temp = 0; gmt = 0; dst = 0;
        WiFi.resetCounters();
        WiFi.setStatus(WL_CONNECTED);
        HTTPClient::setHttpCode(200);
        HTTPClient::setPayload("{\"current_weather\":{\"temperature\":25.5},\"utc_offset_seconds\":3600,\"timezone_abbreviation\":\"CET\"}");
        bool ok = updateLocalWeatherData(temp, gmt, dst);
        RUN_TEST("valid EEPROM+HTTP: returns true", ok == true);
        RUN_TEST("valid EEPROM+HTTP: temp 25.5", float_eq(temp, 25.5f));
        RUN_TEST("valid EEPROM+HTTP: gmt 3600", gmt == 3600);
        RUN_TEST("valid EEPROM+HTTP: dst 0", dst == 0);
    }

    // --- updateLocalWeatherData with WiFi not connected ---
    {
        EEPROM.reset();
        writeStringToEEPROM(LATITUDE_ADDR, "52.0", 15);
        writeStringToEEPROM(LONGITUDE_ADDR, "13.0", 15);
        temp = -1; gmt = -1; dst = -1;
        WiFi.resetCounters();
        WiFi.setStatus(WL_DISCONNECTED);
        bool ok = updateLocalWeatherData(temp, gmt, dst);
        RUN_TEST("no WiFi: returns false", ok == false);
        RUN_TEST("no WiFi: temp unchanged", float_eq(temp, -1.0f));
        RUN_TEST("no WiFi: gmt unchanged", gmt == -1);
        RUN_TEST("no WiFi: dst unchanged", dst == -1);
    }

    // --- updateLocalWeatherData with HTTP error ---
    {
        EEPROM.reset();
        writeStringToEEPROM(LATITUDE_ADDR, "40.0", 15);
        writeStringToEEPROM(LONGITUDE_ADDR, "-74.0", 15);
        temp = 99; gmt = 99; dst = 99;
        WiFi.resetCounters();
        WiFi.setStatus(WL_CONNECTED);
        HTTPClient::setHttpCode(500);
        HTTPClient::setPayload("Server Error");
        bool ok = updateLocalWeatherData(temp, gmt, dst);
        RUN_TEST("HTTP 500: returns false", ok == false);
        RUN_TEST("HTTP 500: temp unchanged", float_eq(temp, 99.0f));
    }

    // --- updateLocalWeatherData with empty EEPROM lat/lon ---
    {
        EEPROM.reset();
        temp = 0; gmt = 0; dst = 0;
        WiFi.resetCounters();
        WiFi.setStatus(WL_CONNECTED);
        HTTPClient::setHttpCode(200);
        HTTPClient::setPayload("{\"current_weather\":{\"temperature\":18.0},\"utc_offset_seconds\":0,\"timezone_abbreviation\":\"UTC\"}");
        bool ok = updateLocalWeatherData(temp, gmt, dst);
        RUN_TEST("empty EEPROM lat/lon: returns true (empty lat/lon pass through)", ok == true);
        RUN_TEST("empty EEPROM lat/lon: temp 18.0", float_eq(temp, 18.0f));
    }

    puts("\n---\nAll weather tests passed!");
    return 0;
}
