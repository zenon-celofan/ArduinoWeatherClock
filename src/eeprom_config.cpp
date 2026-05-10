#include "eeprom_config.h"
#include "eeprom_utils.h"
#include <EEPROM.h>

// --- Brightness ---
int loadBrightness() {
  return EEPROM.read(BRIGHTNESS_ADDR);
}

void saveBrightness(int brightness) {
  EEPROM.write(BRIGHTNESS_ADDR, brightness);
  EEPROM.commit();
}

// --- Display Mode ---
int loadDisplayMode() {
  return EEPROM.read(DISPLAY_MODE_ADDR);
}

void saveDisplayMode(int mode) {
  EEPROM.write(DISPLAY_MODE_ADDR, mode);
  EEPROM.commit();
}

// --- Auto-Update ---
bool loadAutoUpdate() {
  return EEPROM.read(AUTO_UPDATE_ADDR) == 1;
}

void saveAutoUpdate(bool enabled) {
  EEPROM.write(AUTO_UPDATE_ADDR, enabled ? 1 : 0);
  EEPROM.commit();
}

// --- Loki Enabled ---
bool loadLokiEnabled() {
  return EEPROM.read(LOKI_ENABLED_ADDR) == 1;
}

void saveLokiEnabled(bool enabled) {
  EEPROM.write(LOKI_ENABLED_ADDR, enabled ? 1 : 0);
  EEPROM.commit();
}

// --- Time Display Duration ---
int loadTimeDisplayDuration() {
  int duration = EEPROM.read(TIME_DISPLAY_DURATION_ADDR) << 8;
  duration += EEPROM.read(TIME_DISPLAY_DURATION_ADDR + 1);
  return duration;
}

void saveTimeDisplayDuration(int duration) {
  EEPROM.write(TIME_DISPLAY_DURATION_ADDR, duration >> 8);
  EEPROM.write(TIME_DISPLAY_DURATION_ADDR + 1, duration & 0xFF);
  EEPROM.commit();
}

// --- Temperature Display Duration ---
int loadTempDisplayDuration() {
  int duration = EEPROM.read(TEMP_DISPLAY_DURATION_ADDR) << 8;
  duration += EEPROM.read(TEMP_DISPLAY_DURATION_ADDR + 1);
  return duration;
}

void saveTempDisplayDuration(int duration) {
  EEPROM.write(TEMP_DISPLAY_DURATION_ADDR, duration >> 8);
  EEPROM.write(TEMP_DISPLAY_DURATION_ADDR + 1, duration & 0xFF);
  EEPROM.commit();
}

// --- WiFi Credentials ---
bool loadWiFiCredentials(String &ssid, String &password) {
  if (EEPROM.read(FLAG_ADDR) == 1) {
    ssid = readStringFromEEPROM(SSID_ADDR, 32);
    password = readStringFromEEPROM(PASS_ADDR, 32);
    return true;
  }
  return false;
}

void saveWiFiCredentials(const String &ssid, const String &password) {
  writeStringToEEPROM(SSID_ADDR, ssid, 32);
  writeStringToEEPROM(PASS_ADDR, password, 32);
  EEPROM.write(FLAG_ADDR, 1);
  EEPROM.commit();
}

// --- Location Data ---
void saveLocationData(const String &latitude, const String &longitude) {
  writeStringToEEPROM(LATITUDE_ADDR, latitude, 15);
  writeStringToEEPROM(LONGITUDE_ADDR, longitude, 15);
  EEPROM.commit();
}

void loadLocationData(String &latitude, String &longitude) {
  latitude = readStringFromEEPROM(LATITUDE_ADDR, 15);
  longitude = readStringFromEEPROM(LONGITUDE_ADDR, 15);
}
