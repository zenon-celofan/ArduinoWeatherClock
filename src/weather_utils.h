#pragma once

#include <WString.h>

struct WeatherData {
    float temperature;
    long utcOffsetSec;
    int daylightOffsetSec;
};

bool parseOpenMeteoResponse(const String &jsonBody, WeatherData &data);
