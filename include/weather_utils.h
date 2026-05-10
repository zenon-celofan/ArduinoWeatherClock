#pragma once

#include <WString.h>

struct WeatherData {
    float temperature;
    long utcOffsetSec;
    int daylightOffsetSec;
};

bool parseOpenMeteoResponse(const String &jsonBody, WeatherData &data);

bool fetchTemperatureAndTimezone(const String &latitude, const String &longitude,
                                 float &temperature, long &gmtOffsetSec,
                                 int &daylightOffsetSec);

bool updateLocalWeatherData(float &temperature, long &gmtOffsetSec,
                            int &daylightOffsetSec);
