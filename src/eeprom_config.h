#pragma once

#include <WString.h>
#include "eeprom_map.h"

int loadBrightness();
void saveBrightness(int brightness);

int loadDisplayMode();
void saveDisplayMode(int mode);

bool loadAutoUpdate();
void saveAutoUpdate(bool enabled);

bool loadLokiEnabled();
void saveLokiEnabled(bool enabled);

int loadTimeDisplayDuration();
void saveTimeDisplayDuration(int duration);

int loadTempDisplayDuration();
void saveTempDisplayDuration(int duration);

bool loadWiFiCredentials(String &ssid, String &password);
void saveWiFiCredentials(const String &ssid, const String &password);

void loadLocationData(String &latitude, String &longitude);
void saveLocationData(const String &latitude, const String &longitude);
