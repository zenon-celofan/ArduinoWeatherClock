# Arduino Weather Clock

A compact ESP8266-based weather clock that displays the current time and outdoor temperature on a 4-character LED matrix display. It fetches real-time weather data from [Open-Meteo](https://open-meteo.com/) (free, no account needed) and synchronizes time via NTP.

For detailed setup and configuration instructions, see the [User Guide](UserGuide.md).

## Features

- Real-time weather data from Open-Meteo
- NTP time synchronization
- 4-digit LED matrix display (FC16 hardware)
- Configurable display modes (time only, temperature only, or both)
- Web-based configuration interface
- Over-the-air (OTA) firmware updates
- Loki log aggregation support
- Prometheus metrics endpoint (`/metrics`)

## Hardware Required

- ESP8266 (NodeMCU, Wemos D1 Mini, etc.)
- 4-digit LED matrix display (FC16 module)
- Connections:
  - CLK → D5
  - DATA → D7
  - CS → D8

## Setup

1. Power on the device.
2. Connect to the `Clock_AP_XX:XX:XX` WiFi network (password: `12345678`).
3. Open a browser and go to `http://1.2.3.4`.
4. Configure your WiFi and location settings.

## Configuration

The device is configured via a web interface. Settings include:
- WiFi SSID and password
- Latitude and longitude for weather data
- Display brightness (0-15)
- Display mode (time only, temperature only, or both)
- Display durations for time and temperature (in seconds)
- Loki server settings (optional)
- Auto-update on boot (optional)

## API

The device exposes a `/metrics` endpoint for Prometheus monitoring.

## Links

- [Open-Meteo](https://open-meteo.com/)
- [User Guide](UserGuide.md)
- [GitHub Repository](https://github.com/zenon-celofan/ArduinoWeatherClock)
