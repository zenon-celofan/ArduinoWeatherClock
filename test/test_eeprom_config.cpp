#include "eeprom_config.h"
#include "eeprom_utils.h"
#include <EEPROM.h>
#include <cstdio>
#include <cstdlib>

#define RUN_TEST(name, expr) do {                                    \
    printf("  TEST: %s ... ", name);                                 \
    if (!(expr)) { printf("FAIL\n"); return 1; }                     \
    printf("PASS\n");                                                \
} while(0)

static void wipe_eeprom() {
    for (int i = 0; i < 128; i++) EEPROM.write(i, 0);
    EEPROM.commit();
}

int main() {
    puts("\n=== EEPROM Config Tests ===\n");

    EEPROM.begin(128);

    // --- Brightness ---
    wipe_eeprom();
    RUN_TEST("default brightness = 0", loadBrightness() == 0);
    saveBrightness(7);
    RUN_TEST("brightness 7 saved/loaded", loadBrightness() == 7);
    saveBrightness(15);
    RUN_TEST("brightness 15 saved/loaded", loadBrightness() == 15);
    saveBrightness(0);
    RUN_TEST("brightness 0 saved/loaded", loadBrightness() == 0);

    // --- Display Mode ---
    wipe_eeprom();
    RUN_TEST("default displayMode = 0", loadDisplayMode() == 0);
    saveDisplayMode(1);
    RUN_TEST("displayMode 1", loadDisplayMode() == 1);
    saveDisplayMode(2);
    RUN_TEST("displayMode 2", loadDisplayMode() == 2);
    saveDisplayMode(0);
    RUN_TEST("displayMode 0", loadDisplayMode() == 0);

    // --- Auto-Update ---
    wipe_eeprom();
    RUN_TEST("default autoUpdate = false", loadAutoUpdate() == false);
    saveAutoUpdate(true);
    RUN_TEST("autoUpdate true", loadAutoUpdate() == true);
    saveAutoUpdate(false);
    RUN_TEST("autoUpdate false", loadAutoUpdate() == false);

    // --- Loki Enabled ---
    wipe_eeprom();
    RUN_TEST("default lokiEnabled = false", loadLokiEnabled() == false);
    saveLokiEnabled(true);
    RUN_TEST("lokiEnabled true", loadLokiEnabled() == true);
    saveLokiEnabled(false);
    RUN_TEST("lokiEnabled false", loadLokiEnabled() == false);

    // --- Time Display Duration (2 bytes) ---
    wipe_eeprom();
    RUN_TEST("default timeDisplayDuration = 0", loadTimeDisplayDuration() == 0);
    saveTimeDisplayDuration(5);
    RUN_TEST("timeDisplayDuration 5", loadTimeDisplayDuration() == 5);
    saveTimeDisplayDuration(60);
    RUN_TEST("timeDisplayDuration 60", loadTimeDisplayDuration() == 60);
    saveTimeDisplayDuration(1);
    RUN_TEST("timeDisplayDuration 1", loadTimeDisplayDuration() == 1);

    // --- Temp Display Duration (2 bytes) ---
    wipe_eeprom();
    RUN_TEST("default tempDisplayDuration = 0", loadTempDisplayDuration() == 0);
    saveTempDisplayDuration(10);
    RUN_TEST("tempDisplayDuration 10", loadTempDisplayDuration() == 10);
    saveTempDisplayDuration(30);
    RUN_TEST("tempDisplayDuration 30", loadTempDisplayDuration() == 30);

    // --- WiFi Credentials ---
    wipe_eeprom();
    {
        String ssid, pass;
        RUN_TEST("no credentials initially", loadWiFiCredentials(ssid, pass) == false);
    }
    saveWiFiCredentials("MyWiFi", "secret123");
    {
        String ssid, pass;
        RUN_TEST("credentials saved", loadWiFiCredentials(ssid, pass) == true);
        RUN_TEST("SSID matches", ssid == "MyWiFi");
        RUN_TEST("password matches", pass == "secret123");
    }
    // overwrite
    saveWiFiCredentials("NewNet", "newpass");
    {
        String ssid, pass;
        loadWiFiCredentials(ssid, pass);
        RUN_TEST("SSID overwritten", ssid == "NewNet");
        RUN_TEST("password overwritten", pass == "newpass");
    }

    // --- Location Data ---
    wipe_eeprom();
    {
        String lat, lon;
        loadLocationData(lat, lon);
        RUN_TEST("default location empty", lat == "" && lon == "");
    }
    saveLocationData("50.140", "16.955");
    {
        String lat, lon;
        loadLocationData(lat, lon);
        RUN_TEST("latitude saved", lat == "50.140");
        RUN_TEST("longitude saved", lon == "16.955");
    }
    saveLocationData("-33.8688", "151.2093");
    {
        String lat, lon;
        loadLocationData(lat, lon);
        RUN_TEST("latitude overwritten", lat == "-33.8688");
        RUN_TEST("longitude overwritten", lon == "151.2093");
    }

    // --- Verify isolation between config keys ---
    wipe_eeprom();
    saveBrightness(10);
    saveDisplayMode(2);
    saveAutoUpdate(true);
    saveLokiEnabled(true);
    saveTimeDisplayDuration(15);
    saveTempDisplayDuration(20);
    {
        RUN_TEST("isolation: brightness", loadBrightness() == 10);
        RUN_TEST("isolation: displayMode", loadDisplayMode() == 2);
        RUN_TEST("isolation: autoUpdate", loadAutoUpdate() == true);
        RUN_TEST("isolation: lokiEnabled", loadLokiEnabled() == true);
        RUN_TEST("isolation: timeDuration", loadTimeDisplayDuration() == 15);
        RUN_TEST("isolation: tempDuration", loadTempDisplayDuration() == 20);
    }

    puts("\n---\nAll EEPROM config tests passed!");
    return 0;
}
