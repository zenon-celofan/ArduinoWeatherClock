#include "weather_utils.h"
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

    puts("\n---\nAll weather tests passed!");
    return 0;
}
