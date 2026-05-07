# Arduino Weather Clock - User Guide

## What Is It?

A compact ESP8266-based weather clock that displays the current time and outdoor temperature on a 4-character LED matrix display. It fetches real-time weather data from [Open-Meteo](https://open-meteo.com/) (free, no account needed) and synchronizes time via NTP.

---

## Initial Setup

### 1. Power On

Connect the device to a USB power supply. If no WiFi credentials are stored, the device enters **Access Point mode**.

### 2. Connect to the Device

- The display will show `AP`
- Connect your phone or computer to the WiFi network: **`Clock_AP_XX:XX:XX`**
- Password: **`12345678`**

### 3. Open the Configuration Page

- Open your browser and go to **`http://1.2.3.4`**
- The device's IP is also shown briefly on the display after connecting

### 4. Configure Your Settings

Fill in the form (see [Configuration Fields](#configuration-fields) below) and click **Save**. The device will reboot and connect to your WiFi.

---

## Configuration Fields

**WiFi SSID** - Your WiFi network name (e.g. `MyHomeNetwork`)

**WiFi Password** - Your WiFi password (e.g. `secret123`)

**Latitude** - Your latitude for weather data (e.g. `50.140`)

**Longitude** - Your longitude for weather data (e.g. `16.955`)

**Brightness** - LED display brightness, 0 = dimmest, 15 = brightest (e.g. `5`)

**Display Mode** - What to show on the display:
- **Show Both (in loop)** - Alternates between time and temperature
- **Show Time Only** - Only the time is displayed
- **Show Temperature Only** - Only the temperature is displayed

### Display Durations (when in "Show Both" mode)

**Time Display Duration** - How long time is shown in seconds (default: `5`)

**Temp Display Duration** - How long temperature is shown in seconds (default: `5`)

---

## Advanced Options

These options are visible by default but are not required for basic operation.

**Loki IP** - IP address of a Loki log aggregation server (e.g. `192.168.1.100`)

**Loki Port** - Port number for the Loki server (default: `3100`)

**Enable Loki Logging** - Checkbox to enable or disable sending logs to Loki. When unchecked, the IP and Port fields are grayed out and cannot be edited.

---

## Auto Update

**Enable auto-update on boot** - When checked, the device will check for new firmware on GitHub every time it boots. If a newer version is available, it downloads and installs it automatically.

When auto-update is enabled:
- **`+`** on display = update detected, installing
- **`-`** on display = no update available (shown for 1 second)
- **`ERR`** on display = update failed, continuing with current firmware

---

## What You'll See on the Display

- **`AP`** - Device is in Access Point mode (not connected to WiFi)
- **`1205`** - Time: 12:05
- **`+23`** - Temperature: +23°C
- **`-5`** - Temperature: -5°C
- **`wifi`** - WiFi disconnected for more than 60 seconds (blinks for 1s every 10s, alternating with time)
- **`+`** - OTA update in progress
- **`-`** - No OTA update available (1 second)
- **`ERR`** - OTA update failed

---

## WiFi Disconnection Behavior

If WiFi becomes unavailable for **more than 60 seconds**:

1. The display stops showing temperature (data may be stale)
2. Only the time is shown
3. Every 10 seconds, `wifi` appears for 1 second to indicate no connection
4. The device continues trying to reconnect in the background

When WiFi is restored, the display returns to normal operation automatically.

---

## OTA Updates

The device supports **Over-The-Air firmware updates** from GitHub:

1. **Bump** the version number in the firmware
2. **Compile** and create a GitHub release with `ArduinoWeatherClock.bin`
3. **Reboot** the device
4. The device checks for a new version, downloads it, and flashes automatically
5. After flashing, a 10-second sanity timer runs. If the device crashes during this window, the update is rolled back and auto-update is disabled

### Rollback Safety

- If an update causes a crash, the device reboots and retries
- After **2 consecutive failures**, auto-update is automatically disabled to prevent a boot loop

---

## Troubleshooting

- **Display shows `AP`** - Connect to `Clock_AP_XX:XX:XX` and configure WiFi via `http://1.2.3.4`
- **Display shows `wifi`** - WiFi is disconnected. Check your router.
- **Temperature not updating** - Check WiFi connection and coordinates.
- **Auto-update not working** - Re-enable via the web interface checkbox.
- **Cannot access config page** - Make sure your device is on the same WiFi network as the clock.

---

## Finding Your Coordinates

You can find your latitude and longitude at:
- [Google Maps](https://maps.google.com) — right-click on your location
- [Open-Meteo Geocoding](https://open-meteo.com/en/docs/geocoding-api)
