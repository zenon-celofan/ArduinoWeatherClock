#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <EEPROM.h>
#include <time.h>
#include <MD_Parola.h>
#include <MD_MAX72XX.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include "CustomFont.h"
#include <AsyncTimer.h>

// Constants for EEPROM
#define EEPROM_SIZE 127
#define SSID_ADDR 0
#define PASS_ADDR 32
#define FLAG_ADDR 64
#define LATITUDE_ADDR 65
#define LONGITUDE_ADDR 80
#define BRIGHTNESS_ADDR 95
#define DISPLAY_MODE_ADDR 96
#define TIME_DISPLAY_DURATION_ADDR 97
#define TEMP_DISPLAY_DURATION_ADDR 99
#define LOKI_IP1_ADDR 101
#define LOKI_IP2_ADDR 104
#define LOKI_IP3_ADDR 107
#define LOKI_IP4_ADDR 110
#define LOKI_PORT_ADDR 113

// Firmware version and OTA update tracking
#define FIRMWARE_VERSION "0.1.0"
#define UPDATE_PENDING_ADDR 118   // 1 byte: 0=stable, 1=pending verification
#define UPDATE_ATTEMPTS_ADDR 119  // 1 byte: consecutive failed update count
#define MAX_UPDATE_ATTEMPTS 2
#define AUTO_UPDATE_ADDR 120      // 1 byte: 0=disabled, 1=enabled

#define GITHUB_REPO_URL "https://github.com/zenon-celofan/ArduinoWeatherClock"


// Variables for WiFi and WebServer
String ssid = "";
String password = "";

ESP8266WebServer server(80);
bool ntpSyncEnabled = false;
bool apMode = false; // Variable to indicate if AP mode is active

// Variables for Time Configuration
long gmtOffsetSec = 0;
int daylightOffsetSec = 0;
const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
struct tm timeinfo;

// Metrics Variables
unsigned long uptime_seconds = 0;
float localtemp = 0.0; // Variable to store the local temperature
int brightness = 0; // Variable to store the display brightness
int displayMode = 0; // Variable to store the display mode
int timeDisplayDuration = 5; // Variable to store the display duration for time in seconds
int tempDisplayDuration = 5; // Variable to store the display duration for temperature in seconds

// Tickers for periodic tasks
AsyncTimer t;

// Define LED Matrix configuration
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CLK_PIN   D5  // or SCK
#define DATA_PIN  D7  // or MOSI
#define CS_PIN    D8  // or SS

MD_Parola matrixDisplay = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
bool showTime = true; // Variable to toggle between time and temperature

// Global variables for Loki configuration
String lokiIP;
String lokiURL;

// OTA update variables
String latestVersion = "";
String updateUrl = "";

// Forward declarations
bool connectToWiFi(const String &ssid, const String &password);
void displayTemperature();
int semverCompare(const String &v1, const String &v2);
bool checkForUpdates(String &latestTag, String &downloadUrl);

// Compare two semver strings: returns 1 if v1 > v2, -1 if v1 < v2, 0 if equal
int semverCompare(const String &v1, const String &v2) {
  String s1 = v1, s2 = v2;
  s1.replace("v", ""); s2.replace("v", "");
  
  int major1 = 0, minor1 = 0, patch1 = 0;
  int major2 = 0, minor2 = 0, patch2 = 0;
  
  sscanf(s1.c_str(), "%d.%d.%d", &major1, &minor1, &patch1);
  sscanf(s2.c_str(), "%d.%d.%d", &major2, &minor2, &patch2);
  
  if (major1 > major2) return 1;
  if (major1 < major2) return -1;
  if (minor1 > minor2) return 1;
  if (minor1 < minor2) return -1;
  if (patch1 > patch2) return 1;
  if (patch1 < patch2) return -1;
  return 0;
}

// Check GitHub for latest release
bool checkForUpdates(String &latestTag, String &downloadUrl) {
  const String apiUrl = "https://api.github.com/repos/zenon-celofan/ArduinoWeatherClock/releases/latest";
  
  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();
  
  Serial.println("Checking GitHub for updates...");
  http.begin(client, apiUrl);
  http.addHeader("Accept", "application/vnd.github+json");
  http.addHeader("User-Agent", "ESP8266-Weather-Clock");
  
  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("GitHub API error: %d\n", httpCode);
    loki("GitHub API error: " + String(httpCode));
    http.end();
    return false;
  }
  
  String payload = http.getString();
  http.end();
  
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.printf("JSON parse error: %s\n", error.f_str());
    return false;
  }
  
  latestTag = doc["tag_name"].as<String>();
  
  if (doc["assets"].size() > 0) {
    downloadUrl = doc["assets"][0]["browser_download_url"].as<String>();
  }
  
  if (latestTag.length() == 0) {
    Serial.println("No release found on GitHub");
    return false;
  }
  
  int cmp = semverCompare(latestTag, FIRMWARE_VERSION);
  Serial.printf("Current: %s, Latest: %s, Update available: %s\n", 
    FIRMWARE_VERSION, latestTag.c_str(), cmp > 0 ? "YES" : "NO");
  
  return cmp > 0;
}

// Function to initialize Loki configuration
void initLokiConfig() {
  String lokiIP1 = readStringFromEEPROM(LOKI_IP1_ADDR, 3);
  String lokiIP2 = readStringFromEEPROM(LOKI_IP2_ADDR, 3);
  String lokiIP3 = readStringFromEEPROM(LOKI_IP3_ADDR, 3);
  String lokiIP4 = readStringFromEEPROM(LOKI_IP4_ADDR, 3);
  String lokiPort = readStringFromEEPROM(LOKI_PORT_ADDR, 5);

  lokiIP = lokiIP1 + "." + lokiIP2 + "." + lokiIP3 + "." + lokiIP4;
  lokiURL = "http://" + lokiIP + ":" + lokiPort + "/loki/api/v1/push";
}

// Function to send log to Loki server
void loki(const String &logMessage) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Cannot send log to Loki.");
    return;
  }
  if (lokiURL.length() == 0) {
    Serial.println("Loki URL not configured.");
    return;
  }

  HTTPClient http;
  WiFiClient client;

  struct timeval tv;
  gettimeofday(&tv, NULL);

  unsigned long long epochNanoseconds =
      (unsigned long long)(tv.tv_sec) * 1000000000ULL +
      (unsigned long long)(tv.tv_usec) * 1000ULL;

  StaticJsonDocument<512> jsonDoc;
  jsonDoc["streams"][0]["stream"]["job"] = "current_job";
  jsonDoc["streams"][0]["stream"]["level"] = "info";
  jsonDoc["streams"][0]["values"][0][0] = String(epochNanoseconds);
  jsonDoc["streams"][0]["values"][0][1] = logMessage;

  String jsonPayload;
  serializeJson(jsonDoc, jsonPayload);

  Serial.printf("Loki URL: %s\n", lokiURL.c_str());
  Serial.printf("Loki payload: %s\n", jsonPayload.c_str());

  http.begin(client, lokiURL);
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(jsonPayload);
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Log sent to Loki: " + logMessage);
    Serial.println("Response: " + response);
  } else {
    Serial.println("Failed to send log to Loki. HTTP response code: " + String(httpResponseCode));
    if (httpResponseCode < 0) {
      Serial.println("Connection error (negative HTTP code). Check Loki server connectivity.");
    }
  }
  http.end();
}

void reconnectWifi() {
  // Check Wi-Fi connection status
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi disconnected. Attempting to reconnect...");
    connectToWiFi(ssid, password);
  } else {
    Serial.println("Wi-Fi connected.");
  }
}

// Save location data to EEPROM
void saveLocationData(const String &latitude, const String &longitude) {
  writeStringToEEPROM(LATITUDE_ADDR, latitude, 15);
  writeStringToEEPROM(LONGITUDE_ADDR, longitude, 15);
  EEPROM.commit();
}

// Load location data from EEPROM
void loadLocationData(String &latitude, String &longitude) {
  latitude = readStringFromEEPROM(LATITUDE_ADDR, 15);
  longitude = readStringFromEEPROM(LONGITUDE_ADDR, 15);
}

// Read a string from EEPROM
String readStringFromEEPROM(int startAddr, int length) {
  char data[length + 1];
  for (int i = 0; i < length; i++) {
    data[i] = EEPROM.read(startAddr + i);
  }
  data[length] = '\0';
  return String(data);
}

// Write a string to EEPROM
void writeStringToEEPROM(int startAddr, const String &str, int maxLength) {
  for (int i = 0; i < maxLength; i++) {
    if (i < str.length()) {
      EEPROM.write(startAddr + i, str[i]);
    } else {
      EEPROM.write(startAddr + i, 0);
    }
  }
}

// Load WiFi credentials from EEPROM
bool loadWiFiCredentials(String &ssid, String &password) {
  if (EEPROM.read(FLAG_ADDR) == 1) {
    ssid = readStringFromEEPROM(SSID_ADDR, 32);
    password = readStringFromEEPROM(PASS_ADDR, 32);
    return true;
  }
  return false;
}

// Save WiFi credentials to EEPROM
void saveWiFiCredentials(const String &ssid, const String &password) {
  writeStringToEEPROM(SSID_ADDR, ssid, 32);
  writeStringToEEPROM(PASS_ADDR, password, 32);
  EEPROM.write(FLAG_ADDR, 1);
  EEPROM.commit();
}

// Load brightness from EEPROM
int loadBrightness() {
  return EEPROM.read(BRIGHTNESS_ADDR);
}

// Save brightness to EEPROM
void saveBrightness(int brightness) {
  EEPROM.write(BRIGHTNESS_ADDR, brightness);
  EEPROM.commit();
}

// Load display mode from EEPROM
int loadDisplayMode() {
  return EEPROM.read(DISPLAY_MODE_ADDR);
}

// Save display mode to EEPROM
void saveDisplayMode(int mode) {
  EEPROM.write(DISPLAY_MODE_ADDR, mode);
  EEPROM.commit();
}

// Load auto-update setting from EEPROM
bool loadAutoUpdate() {
  byte val = EEPROM.read(AUTO_UPDATE_ADDR);
  Serial.printf("loadAutoUpdate - read byte at %d: %d, returning %s\n",
    AUTO_UPDATE_ADDR, val, val == 1 ? "true" : "false");
  return val == 1;
}

void saveAutoUpdate(bool enabled) {
  Serial.printf("saveAutoUpdate - writing %d to addr %d\n", enabled ? 1 : 0, AUTO_UPDATE_ADDR);
  EEPROM.write(AUTO_UPDATE_ADDR, enabled ? 1 : 0);
  EEPROM.commit();
}

// Load time display duration from EEPROM
int loadTimeDisplayDuration() {
  int duration = EEPROM.read(TIME_DISPLAY_DURATION_ADDR) << 8;
  duration += EEPROM.read(TIME_DISPLAY_DURATION_ADDR + 1);
  return duration;
}

// Save time display duration to EEPROM
void saveTimeDisplayDuration(int duration) {
  EEPROM.write(TIME_DISPLAY_DURATION_ADDR, duration >> 8);
  EEPROM.write(TIME_DISPLAY_DURATION_ADDR + 1, duration & 0xFF);
  EEPROM.commit();
}

// Load temperature display duration from EEPROM
int loadTempDisplayDuration() {
  int duration = EEPROM.read(TEMP_DISPLAY_DURATION_ADDR) << 8;
  duration += EEPROM.read(TEMP_DISPLAY_DURATION_ADDR + 1);
  return duration;
}

// Save temperature display duration to EEPROM
void saveTempDisplayDuration(int duration) {
  EEPROM.write(TEMP_DISPLAY_DURATION_ADDR, duration >> 8);
  EEPROM.write(TEMP_DISPLAY_DURATION_ADDR + 1, duration & 0xFF);
  EEPROM.commit();
}

// Serve the configuration page
void serveConfigPage() {
  String ssid = WiFi.SSID();
  String password = readStringFromEEPROM(PASS_ADDR, 32);
  String latitude = readStringFromEEPROM(LATITUDE_ADDR, 15);
  String longitude = readStringFromEEPROM(LONGITUDE_ADDR, 15);
  String brightness = String(loadBrightness());
  String displayMode = String(loadDisplayMode());
  String timeDisplayDuration = String(loadTimeDisplayDuration());
  String tempDisplayDuration = String(loadTempDisplayDuration());
  String lokiIP1 = readStringFromEEPROM(LOKI_IP1_ADDR, 3);
  String lokiIP2 = readStringFromEEPROM(LOKI_IP2_ADDR, 3);
  String lokiIP3 = readStringFromEEPROM(LOKI_IP3_ADDR, 3);
  String lokiIP4 = readStringFromEEPROM(LOKI_IP4_ADDR, 3);
  String lokiPort = readStringFromEEPROM(LOKI_PORT_ADDR, 5);
  bool autoUpdate = loadAutoUpdate();
  Serial.printf("Config page loaded - auto_update from EEPROM: %s (addr %d)\n",
    autoUpdate ? "true" : "false", AUTO_UPDATE_ADDR);

  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
        <title>Clock Configuration</title>
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <style>
            body { 
                font-family: Arial, sans-serif; 
                margin: 0; 
                padding: 0; 
                display: flex; 
                flex-direction: column; 
                align-items: center; 
                justify-content: center; 
                height: 100vh; 
                background-color: #f5f5f5; 
                margin-top: 200px;
                margin-bottom: 200px;
            }
            h1 { 
                color: #333; 
                font-size: 24px; 
                margin-bottom: 20px; 
            }
            form {
                padding: 20px; 
                width: 90%; 
                max-width: 400px; 
                box-sizing: border-box;
            }
            input, select {
                width: calc(100% - 20px); 
                padding: 10px; 
                margin: 10px 0; 
                border: 1px solid #ccc; 
                border-radius: 5px; 
                font-size: 16px;
            }
            input[type="number"] {
                -moz-appearance: textfield;
            }
            input[type="number"]::-webkit-outer-spin-button,
            input[type="number"]::-webkit-inner-spin-button {
                -webkit-appearance: none;
                margin: 0;
            }
            button {
                width: 100%; 
                padding: 10px; 
                background: #007BFF; 
                color: #fff; 
                border: none; 
                border-radius: 5px; 
                font-size: 16px; 
                cursor: pointer; 
                transition: background-color 0.3s ease;
            }
            button:hover {
                background-color: #0056b3;
            }
            .field-description {
                font-size: 14px; 
                color: #777; 
                margin-top: -10px; 
                margin-bottom: 10px;
            }
            #advancedOptions {
                display: block;
            }
            .ip-part {
                width: 14%; 
                display: inline-block; 
                margin-right: 2%;
            }
            .checkbox-row {
                display: flex;
                align-items: center;
                margin: 10px 0;
            }
            .checkbox-row input[type="checkbox"] {
                width: auto;
                margin-right: 8px;
                margin-top: 0;
                margin-bottom: 0;
            }
            .checkbox-row label {
                font-size: 16px;
                color: #333;
            }
            .repo-link {
                font-size: 14px;
                color: #007BFF;
                margin-top: -5px;
                margin-bottom: 10px;
            }
            .update-section {
                margin-top: 15px;
                padding-top: 10px;
                border-top: 1px solid #ddd;
            }
        </style>
    </head>
    <body>
        <h1>Clock Configuration</h1>
        <form action="/save" method="POST">
            <input type="text" name="ssid" value=")rawliteral" + ssid + R"rawliteral(" placeholder="WiFi SSID" required>
            <div class="field-description">Enter the SSID of your WiFi network.</div>
            <input type="password" name="password" value=")rawliteral" + password + R"rawliteral(" placeholder="WiFi Password" required>
            <div class="field-description">Enter the password for your WiFi network.</div>
            <input type="number" name="latitude" value=")rawliteral" + latitude + R"rawliteral(" placeholder="Latitude" step=0.0001 required>
            <div class="field-description">Enter the latitude of your location.</div>
            <input type="number" name="longitude" value=")rawliteral" + longitude + R"rawliteral(" placeholder="Longitude" step=0.0001 required>
            <div class="field-description">Enter the longitude of your location.</div>
            <input type="number" name="brightness" value=")rawliteral" + brightness + R"rawliteral(" min="0" max="15" required>
            <div class="field-description">Set the brightness level for the LED display (0 = dimmest, 15 = brightest).</div>
            <select name="display_mode">
                <option value="0")rawliteral" + (displayMode == "0" ? " selected" : "") + R"rawliteral(">Show Both (in loop)</option>
                <option value="1")rawliteral" + (displayMode == "1" ? " selected" : "") + R"rawliteral(">Show Time Only</option>
                <option value="2")rawliteral" + (displayMode == "2" ? " selected" : "") + R"rawliteral(">Show Temperature Only</option>
            </select>
            <div class="field-description">Select the display mode for the LED display.</div>
            <input type="number" name="time_display_duration" value=")rawliteral" + timeDisplayDuration + R"rawliteral(" min="1" max="60" required>
            <div class="field-description">Set the duration to display the time in seconds.</div>
            <input type="number" name="temp_display_duration" value=")rawliteral" + tempDisplayDuration + R"rawliteral(" min="1" max="60" required>
            <div class="field-description">Set the duration to display the temperature in seconds.</div>
            <div id="advancedOptions">
                <div>Advanced Options:</div>
                <input type="number" class="ip-part" name="loki_ip1" value=")rawliteral" + lokiIP1 + R"rawliteral(" min="0" max="255" required>
                <input type="number" class="ip-part" name="loki_ip2" value=")rawliteral" + lokiIP2 + R"rawliteral(" min="0" max="255" required>
                <input type="number" class="ip-part" name="loki_ip3" value=")rawliteral" + lokiIP3 + R"rawliteral(" min="0" max="255" required>
                <input type="number" class="ip-part" name="loki_ip4" value=")rawliteral" + lokiIP4 + R"rawliteral(" min="0" max="255" required>
                <div class="field-description">Enter the IP address of the Loki server.</div>
                <input type="number" name="loki_port" value=")rawliteral" + lokiPort + R"rawliteral(" placeholder="Loki Server Port" required>
                <div class="field-description">Enter the port for the Loki server.</div>
            </div>
            <div class="update-section">
                <div>Auto Update:</div>
                <div class="checkbox-row">
                    <input type="checkbox" id="auto_update" name="auto_update" value="1" )rawliteral" + (autoUpdate ? "checked" : "") + R"rawliteral(">
                    <label for="auto_update">Enable auto-update on boot</label>
                </div>
                <div class="field-description">When enabled, the clock will check for new firmware on reboot and update automatically.</div>
                <div class="repo-link">Source: <a href=")rawliteral" + GITHUB_REPO_URL + R"rawliteral(" target="_blank">)rawliteral" + GITHUB_REPO_URL + R"rawliteral(</a></div>
            </div>
            <button type="submit">Save</button>
        </form>
    </body>
    </html>
  )rawliteral";

  server.send(200, "text/html", html);
}


// Handle saving WiFi credentials, location data, brightness, display mode, and display times
void handleSaveConfig() {
  if (server.method() == HTTP_POST) {
    Serial.println("=== FORM POST RECEIVED ===");
    for (int i = 0; i < server.args(); i++) {
      Serial.printf("Param %d: name='%s', value='%s'\n", i, server.argName(i).c_str(), server.arg(i).c_str());
    }
    Serial.println("==========================");
    
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    String latitude = server.arg("latitude");
    String longitude = server.arg("longitude");
    int brightness = server.arg("brightness").toInt();
    int displayMode = server.arg("display_mode").toInt();
    int timeDisplayDuration = server.arg("time_display_duration").toInt();
    int tempDisplayDuration = server.arg("temp_display_duration").toInt();
    String lokiIP1 = server.arg("loki_ip1");
    String lokiIP2 = server.arg("loki_ip2");
    String lokiIP3 = server.arg("loki_ip3");
    String lokiIP4 = server.arg("loki_ip4");
    String lokiPort = server.arg("loki_port");
    bool autoUpdate = server.hasArg("auto_update");

    Serial.printf("Form received - auto_update param: '%s', parsed: %s\n",
      server.arg("auto_update").c_str(), autoUpdate ? "true" : "false");

    saveWiFiCredentials(ssid, password);
    saveLocationData(latitude, longitude);
    saveBrightness(brightness);
    saveDisplayMode(displayMode);
    saveTimeDisplayDuration(timeDisplayDuration);
    saveTempDisplayDuration(tempDisplayDuration);
    saveAutoUpdate(autoUpdate);
    
    // Save Loki server configuration
    writeStringToEEPROM(LOKI_IP1_ADDR, lokiIP1, 3);
    writeStringToEEPROM(LOKI_IP2_ADDR, lokiIP2, 3);
    writeStringToEEPROM(LOKI_IP3_ADDR, lokiIP3, 3);
    writeStringToEEPROM(LOKI_IP4_ADDR, lokiIP4, 3);
    writeStringToEEPROM(LOKI_PORT_ADDR, lokiPort, 5);
    EEPROM.commit();

    server.send(200, "text/html", "<h1>Configuration Saved!</h1><p>Rebooting...</p>");
    delay(2000);
    ESP.restart();
  } else {
    server.send(405, "text/plain", "Method Not Allowed");
  }
}

// Serve metrics for Prometheus
void serveMetrics() {
  getLocalTime(&timeinfo);
  String metrics = 
    "# HELP uptime_seconds The number of seconds the system has been running.\n"
    "# TYPE uptime_seconds counter\n"
    "uptime_seconds " + String(uptime_seconds) + "\n"
    "\n"
    "# HELP free_heap_mem_bytes Free heap memory left.\n"
    "# TYPE free_heap_mem_bytes gauge\n"
    "free_heap_mem_bytes " + String(system_get_free_heap_size()) + "\n"
    "\n"
    "# HELP localtime_hours The current hour of the day in 24-hour format.\n"
    "# TYPE localtime_hours gauge\n"
    "localtime_hours " + String(timeinfo.tm_hour) + "\n"
    "\n"    
    "# HELP localtime_minutes The current minute of the hour.\n"
    "# TYPE localtime_minutes gauge\n"
    "localtime_minutes " + String(timeinfo.tm_min) + "\n"
    "\n"
    "# HELP localtime_seconds The current second of the minute.\n"
    "# TYPE localtime_seconds gauge\n"
    "localtime_seconds " + String(timeinfo.tm_sec) + "\n"
    "\n"
    "# HELP localtemp The current local temperature in Celsius.\n"
    "# TYPE localtemp gauge\n"
    "localtemp " + String(localtemp) + "\n"
    "\n"
    "# HELP brightness The current brightness level of the LED display.\n"
    "# TYPE brightness gauge\n"
    "brightness " + String(brightness) + "\n"
    "\n"
    "# HELP display_mode The current display mode of the LED display (0 = both, 1 = time only, 2 = temperature only).\n"
    "# TYPE display_mode gauge\n"
    "display_mode " + String(displayMode) + "\n"
    "\n"
    "# HELP time_display_duration The duration to display the time in seconds.\n"
    "# TYPE time_display_duration gauge\n"
    "time_display_duration " + String(timeDisplayDuration) + "\n"
    "\n"
    "# HELP temp_display_duration The duration to display the temperature in seconds.\n"
    "# TYPE temp_display_duration gauge\n"
    "temp_display_duration " + String(tempDisplayDuration) + "\n";
  server.send(200, "text/plain", metrics);
}

// Start the Web Server
void startWebServer() {
  server.on("/", HTTP_GET, serveConfigPage);
  server.on("/save", HTTP_POST, handleSaveConfig);
  server.on("/metrics", HTTP_GET, serveMetrics);
  server.begin();
  Serial.println("Web server started.");
}

// Connect to WiFi using stored credentials
bool connectToWiFi(const String &ssid, const String &password) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.print("Connecting to WiFi");

  for (int i = 0; i < 20; i++) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected!");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      return true;
    }
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nFailed to connect.");
  return false;
}

// Fetch temperature and timezone from Open-Meteo API
bool fetchTemperatureAndTimezone(const String &latitude, const String &longitude, float &temperature, long &gmtOffsetSec, int &daylightOffsetSec) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClient client;
    String url = "http://api.open-meteo.com/v1/forecast?latitude=" + latitude + "&longitude=" + longitude + "&current_weather=true&timezone=auto";
    Serial.print("Request URL: ");
    Serial.println(url);
    loki("Request: " + url);
    http.begin(client, url);
    Serial.println("http.begin - ok");
    int httpCode = http.GET();
    Serial.println("GET - ok");
    Serial.print("Http response code: ");
    Serial.println(httpCode);
    loki("HTTP response code: " + String(httpCode));
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.print("Response: ");
      Serial.println(payload);
      loki("Response: " + payload);
      
      // Parse JSON response
      StaticJsonDocument<1024> doc;
      DeserializationError error = deserializeJson(doc, payload);
      if (error) {
        Serial.print("JSON deserialization failed: ");
        Serial.println(error.f_str()); return false;
      }
      
      // Extract temperature from parsed JSON
      temperature = doc["current_weather"]["temperature"];
      
      // Extract timezone info from parsed JSON
      gmtOffsetSec = doc["utc_offset_seconds"];
      daylightOffsetSec = doc["timezone_abbreviation"] == "DST" ? 3600 : 0; http.end();
      return true;
    } else {
      Serial.print("HTTP error: ");
      Serial.println(httpCode);
      loki("HTTP error: " + String(httpCode));
    }
    http.end();
  }
  reconnectWifi();
  return false;
}

// Update local temperature and timezone from the internet
void updateLocalTemperatureAndTimezone() {
  if (apMode) return; // Do not update if in AP mode
  String latitude, longitude;
  loadLocationData(latitude, longitude);
  if (fetchTemperatureAndTimezone(latitude, longitude, localtemp, gmtOffsetSec, daylightOffsetSec)) {
    Serial.print("Temperature: ");
    Serial.println(localtemp);
    Serial.print("GMT Offset: ");
    Serial.println(gmtOffsetSec);
    Serial.print("Daylight Saving Offset: ");
    Serial.println(daylightOffsetSec);
    configTime(gmtOffsetSec, daylightOffsetSec, ntpServer1, ntpServer2);
  } else {
    Serial.println("Failed to fetch temperature and timezone.");
    loki("Failed to fetch temperature and timezone.");
  }
}

// Refresh display
void refreshDisplay() {
  uptime_seconds++;
  char displayStr[8];
  if (apMode) {
    matrixDisplay.setTextAlignment(PA_CENTER);
    snprintf(displayStr, sizeof(displayStr), "AP");
  } else if (displayMode == 1 || (displayMode == 0 && showTime)) {
    getLocalTime(&timeinfo);
    matrixDisplay.setTextAlignment(PA_RIGHT);
    snprintf(displayStr, sizeof(displayStr), "%d%02d", timeinfo.tm_hour, timeinfo.tm_min);
  } else {
    int temp = round(localtemp);
    matrixDisplay.setTextAlignment(PA_CENTER);
    snprintf(displayStr, sizeof(displayStr), temp > 0 ? "+%d" : "%d", temp);
  }
  matrixDisplay.setFont(BigFontNew); // Use the custom font
  matrixDisplay.setIntensity(brightness); // Set display brightness
  matrixDisplay.print(displayStr);
}

// Display time
void displayTime() {
  showTime = true;
  refreshDisplay();
  t.setTimeout(displayTemperature, timeDisplayDuration * 1000); // timeDisplayTicker
}

// Display temperature
void displayTemperature() {
  showTime = false;
  refreshDisplay();
  t.setTimeout(displayTime, tempDisplayDuration * 1000);  // tempDisplayTicker
}

// Display IP address in parts
void displayIPAddress() {
  IPAddress ip = WiFi.localIP();
  for (int i = 0; i < 4; i++) {
    char partStr[4];
    snprintf(partStr, sizeof(partStr), "%03d", ip[i]);
    if (i < 3) {
      strcat(partStr, ".");
    }
    matrixDisplay.setTextAlignment(PA_CENTER);
    matrixDisplay.setFont(BigFontNew); // Use the custom font
    matrixDisplay.print(partStr); delay(1000);
  }
}

void printFreeHeapSize() {
  Serial.println("Free memory: " + String(system_get_free_heap_size()) + " bytes");
}


void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);

  // Initialize Loki configuration
  initLokiConfig();

  // Get the MAC address
  String macAddress = WiFi.macAddress();
  Serial.print("MAC Address: ");
  Serial.println(macAddress);

  // Initialize LED Matrix display
  matrixDisplay.begin();
  matrixDisplay.setCharSpacing(0);
  matrixDisplay.displayClear();

  // Load configuration from EEPROM
  brightness = loadBrightness();
  displayMode = loadDisplayMode();
  timeDisplayDuration = loadTimeDisplayDuration();
  tempDisplayDuration = loadTempDisplayDuration();

  byte auVal = EEPROM.read(AUTO_UPDATE_ADDR);
  Serial.printf("EEPROM AUTO_UPDATE_ADDR(%d) = %d\n", AUTO_UPDATE_ADDR, auVal);

  matrixDisplay.setIntensity(brightness); // Set initial brightness level

  String ssid, password;
  if (loadWiFiCredentials(ssid, password) && connectToWiFi(ssid, password)) {
    Serial.println("WiFi connected using stored credentials.");
    ntpSyncEnabled = true;
    displayIPAddress(); // Display IP address in parts
  } else {
    Serial.println("Starting Access Point for configuration...");

    String lastThreeChunks = macAddress.substring(macAddress.length() - 8);
    String ssid = "Clock_AP_" + lastThreeChunks;
    String password = "12345678"; // Set your AP password here

    // Custom IP configuration
    IPAddress apIP(1, 2, 3, 4); // Set your desired IP address here
    IPAddress gateway(1, 2, 3, 4); // Gateway is usually the same as the IP
    IPAddress subnet(255, 255, 255, 0); // Subnet mask

    WiFi.softAPConfig(apIP, gateway, subnet); // Apply the custom IP configuration

    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid.c_str(), password.c_str());

    Serial.print("Access Point SSID: ");
    Serial.println(ssid);
    Serial.print("Access Point Password: ");
    Serial.println(password);
    Serial.print("Access Point IP address: ");
    Serial.println(WiFi.softAPIP());

    apMode = true;
    ntpSyncEnabled = false;
  }

  // Load and print location data
  String latitude, longitude;
  loadLocationData(latitude, longitude);
  Serial.print("Latitude: ");
  Serial.println(latitude);
  Serial.print("Longitude: ");
  Serial.println(longitude);

  startWebServer();
  if (!apMode) {
    updateLocalTemperatureAndTimezone();
    t.setInterval(updateLocalTemperatureAndTimezone, 600 * 1000);  // tempUpdateTicker
  }
  t.setInterval(refreshDisplay, 1 * 1000);
  if (displayMode == 0) {
    displayTime();
  } else if (displayMode == 1) {
    showTime = true;
    t.setInterval(refreshDisplay, 1 * 1000);
  } else if (displayMode == 2) {
    showTime = false;
    t.setInterval(refreshDisplay, 1 * 1000);
  }

  t.setInterval(printFreeHeapSize, 60 * 1000);

  loki("setup() completed.");
}


void loop() {
  server.handleClient();
  matrixDisplay.displayAnimate();

  if (WiFi.getMode() != WIFI_AP && WiFi.status() == WL_CONNECTED && !ntpSyncEnabled) {
    Serial.println("Re-enabling NTP synchronization...");
    configTime(gmtOffsetSec, daylightOffsetSec, ntpServer1, ntpServer2);
    ntpSyncEnabled = true;
  }
  t.handle();
}
 