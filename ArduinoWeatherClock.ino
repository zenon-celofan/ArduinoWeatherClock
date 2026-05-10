#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <EEPROM.h>
#include <time.h>
#include <MD_Parola.h>
#include <MD_MAX72XX.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include "CustomFont.h"
#include <AsyncTimer.h>
#include <WiFiClientSecureBearSSL.h>

#include "src/semver_compare.h"
#include "src/eeprom_utils.h"
#include "src/eeprom_map.h"
#include "src/eeprom_config.h"
#include "src/temp_utils.h"
#include "src/url_utils.h"
#include "src/device_utils.h"
#include "src/metrics_utils.h"
#include "src/display_utils.h"
#include "src/update_utils.h"
#include "src/loki_utils.h"

// Firmware version and OTA update tracking
#define FIRMWARE_VERSION "0.1.28"
#define MAX_UPDATE_ATTEMPTS 2

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

// WiFi connectivity tracking
unsigned long lastWifiConnectedAt = 0;
unsigned long wifiDisconnectedSince = 0;
bool wifiWasEverConnected = false;

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
String deviceName;
String lastDisplayContent;

// OTA update variables
String latestVersion = "";
String updateUrl = "";

// Forward declarations
bool connectToWiFi(const String &ssid, const String &password);
void displayTemperature();
void loki(const String &category, const String &logMessage);
bool checkForUpdates(String &latestTag, String &downloadUrl);
bool performOTAUpdate(const String &url);
void handleUpdateBootCheck();

// Check GitHub for latest release
bool checkForUpdates(String &latestTag, String &downloadUrl) {
  BearSSL::WiFiClientSecure client;
  client.setInsecure();
  client.setBufferSizes(512, 512);
  
  const char* host = "api.github.com";
  String request = "GET /repos/zenon-celofan/ArduinoWeatherClock/releases/latest HTTP/1.1\r\n"
                   "Host: api.github.com\r\n"
                   "User-Agent: ESP8266-Weather-Clock\r\n"
                   "Accept: application/vnd.github+json\r\n"
                   "Connection: close\r\n\r\n";
  
  Serial.println("Connecting to GitHub API...");
  if (!client.connect(host, 443)) {
    Serial.println("Connection to GitHub failed");
    return false;
  }
  
  client.print(request);
  Serial.println("Request sent, waiting for response...");
  
  unsigned long timeout = millis();
  while (!client.available() && millis() - timeout < 10000) {
    delay(10);
  }
  
  if (!client.available()) {
    Serial.println("Timeout waiting for response");
    client.stop();
    return false;
  }
  
  while (client.available()) {
    String line = client.readStringUntil('\n');
    if (line == "\r" || line.length() == 0) break;
  }
  
  String body;
  while (client.available()) {
    body += client.readStringUntil('\n');
    body += "\n";
    yield();
  }
  client.stop();
  
  Serial.printf("Response body length: %d\n", body.length());
  Serial.printf("First 300 chars: %s\n", body.substring(0, 300).c_str());

  bool hasUpdate = parseGitHubRelease(body, latestTag, downloadUrl, FIRMWARE_VERSION);

  Serial.printf("Current: %s, Latest: %s, Update available: %s\n",
    FIRMWARE_VERSION, latestTag.c_str(), hasUpdate ? "YES" : "NO");

  return hasUpdate;
}

// Perform OTA update by downloading firmware and flashing manually
bool performOTAUpdate(const String &url) {
  Serial.printf("Starting OTA update from: %s\n", url.c_str());

  BearSSL::WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.begin(client, url);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(120000);

  int httpCode = http.GET();
  Serial.printf("Download HTTP code: %d\n", httpCode);

  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("Download failed: %d\n", httpCode);
    http.end();
    return false;
  }

  int contentLength = http.getSize();
  Serial.printf("Firmware size: %d bytes\n", contentLength);

  if (contentLength <= 0) {
    Serial.println("Invalid content length");
    http.end();
    return false;
  }

  WiFiClient *stream = http.getStreamPtr();

  if (!Update.begin(contentLength, U_FLASH)) {
    Serial.printf("Update.begin failed: %s\n", Update.getErrorString());
    http.end();
    return false;
  }

  Serial.println("Flashing firmware...");
  uint8_t buff[1024] = { 0 };
  size_t totalWritten = 0;

  unsigned long start = millis();
  while (http.connected() && totalWritten < (size_t)contentLength) {
    if (millis() - start > 60000) {
      Serial.println("Download timeout");
      break;
    }
    size_t avail = stream->available();
    if (avail) {
      size_t toRead = (avail > sizeof(buff)) ? sizeof(buff) : avail;
      size_t read = stream->readBytes(buff, toRead);
      Update.write(buff, read);
      totalWritten += read;
      start = millis();
      if (totalWritten % 50000 < read) {
        Serial.printf("  Progress: %d/%d bytes\n", totalWritten, contentLength);
      }
    }
    yield();
  }
  http.end();

  if (Update.end(true)) {
    Serial.printf("Flash complete: %d bytes written\n", totalWritten);
    return true;
  } else {
    Serial.printf("Flash failed: %s\n", Update.getErrorString());
    return false;
  }
}

// Handle boot-time update check and rollback
void handleUpdateBootCheck() {
  byte pending = EEPROM.read(UPDATE_PENDING_ADDR);
  byte attempts = EEPROM.read(UPDATE_ATTEMPTS_ADDR);

  if (pending == 1) {
    Serial.printf("Update pending detected (attempts: %d)\n", attempts);

    if (attempts >= MAX_UPDATE_ATTEMPTS) {
      Serial.println("Max update attempts reached, disabling auto-update");
      EEPROM.write(AUTO_UPDATE_ADDR, 0);
      EEPROM.write(UPDATE_PENDING_ADDR, 0);
      EEPROM.write(UPDATE_ATTEMPTS_ADDR, 0);
      EEPROM.commit();
      return;
    }

    EEPROM.write(UPDATE_ATTEMPTS_ADDR, attempts + 1);
    EEPROM.commit();

    Serial.printf("Starting 10s sanity timer (attempt %d/%d)...\n",
      attempts + 1, MAX_UPDATE_ATTEMPTS);

    unsigned long start = millis();
    while (millis() - start < 10000) {
      server.handleClient();
      matrixDisplay.displayAnimate();
      t.handle();
    }

    Serial.println("Sanity timer passed - marking update as verified");
    EEPROM.write(UPDATE_PENDING_ADDR, 0);
    EEPROM.write(UPDATE_ATTEMPTS_ADDR, 0);
    EEPROM.commit();
  } else {
    EEPROM.write(UPDATE_ATTEMPTS_ADDR, 0);
    EEPROM.commit();
  }
}

// Function to initialize Loki configuration
void initLokiConfig() {
  String lokiIP1 = readStringFromEEPROM(LOKI_IP1_ADDR, 3);
  String lokiIP2 = readStringFromEEPROM(LOKI_IP2_ADDR, 3);
  String lokiIP3 = readStringFromEEPROM(LOKI_IP3_ADDR, 3);
  String lokiIP4 = readStringFromEEPROM(LOKI_IP4_ADDR, 3);
  String lokiPort = readStringFromEEPROM(LOKI_PORT_ADDR, 5);

  lokiIP = lokiIP1 + "." + lokiIP2 + "." + lokiIP3 + "." + lokiIP4;
  lokiURL = buildLokiUrl(lokiIP, lokiPort);
}

// Function to send log to Loki server
void loki(const String &category, const String &logMessage) {
  if (!loadLokiEnabled()) return;
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  if (lokiURL.length() == 0) {
    return;
  }

  struct timeval tv;
  gettimeofday(&tv, NULL);

  if (!isTimestampValid((unsigned long long)tv.tv_sec)) {
    return;
  }

  unsigned long long epochNanoseconds =
      (unsigned long long)(tv.tv_sec) * 1000000000ULL +
      (unsigned long long)(tv.tv_usec) * 1000ULL;

  String jsonPayload = buildLokiPayload(deviceName, category, logMessage, epochNanoseconds);

  HTTPClient http;
  WiFiClient client;
  http.begin(client, lokiURL);
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(jsonPayload);
  String response = http.getString();
  if (httpResponseCode == 204 || httpResponseCode == 200) {
    Serial.println("Loki OK [HTTP " + String(httpResponseCode) + "]: " + logMessage);
  } else {
    Serial.println("Loki FAILED [HTTP " + String(httpResponseCode) + "]: " + logMessage + " | Response: " + response);
    if (httpResponseCode < 0) {
      Serial.println("Connection error (negative HTTP code). Check Loki server connectivity.");
    }
  }
  http.end();
}

void reconnectWifi() {
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiDisconnectedSince == 0) wifiDisconnectedSince = millis();
    Serial.println("Wi-Fi disconnected. Attempting to reconnect...");
    loki("wifi", "Attempting WiFi reconnection");
    connectToWiFi(ssid, password);
  } else {
    if (wifiDisconnectedSince > 0) {
      Serial.printf("Wi-Fi reconnected after %lu seconds\n", (millis() - wifiDisconnectedSince) / 1000);
      loki("wifi", "WiFi reconnected after " + String((millis() - wifiDisconnectedSince) / 1000) + "s");
    }
    lastWifiConnectedAt = millis();
    wifiDisconnectedSince = 0;
  }
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
  bool lokiEnabled = loadLokiEnabled();
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
            .loki-fields.disabled input {
                background-color: #e9ecef;
                color: #6c757d;
                pointer-events: none;
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
                <input type="hidden" name="loki_enabled" value="0">
                <div class="checkbox-row">
                    <input type="checkbox" id="loki_enabled" name="loki_enabled" value="1")rawliteral" + (lokiEnabled ? " checked" : "") + R"rawliteral(>
                    <label for="loki_enabled">Enable Loki logging</label>
                </div>
                <div id="lokiFields" class="loki-fields)rawliteral" + (lokiEnabled ? "" : " disabled") + R"rawliteral(">
                    <input type="number" class="ip-part" name="loki_ip1" value=")rawliteral" + lokiIP1 + R"rawliteral(" min="0" max="255" required )rawliteral" + (lokiEnabled ? "" : "disabled") + R"rawliteral(>
                    <input type="number" class="ip-part" name="loki_ip2" value=")rawliteral" + lokiIP2 + R"rawliteral(" min="0" max="255" required )rawliteral" + (lokiEnabled ? "" : "disabled") + R"rawliteral(>
                    <input type="number" class="ip-part" name="loki_ip3" value=")rawliteral" + lokiIP3 + R"rawliteral(" min="0" max="255" required )rawliteral" + (lokiEnabled ? "" : "disabled") + R"rawliteral(>
                    <input type="number" class="ip-part" name="loki_ip4" value=")rawliteral" + lokiIP4 + R"rawliteral(" min="0" max="255" required )rawliteral" + (lokiEnabled ? "" : "disabled") + R"rawliteral(>
                    <div class="field-description">Enter the IP address of the Loki server.</div>
                    <input type="number" name="loki_port" value=")rawliteral" + lokiPort + R"rawliteral(" placeholder="Loki Server Port" required )rawliteral" + (lokiEnabled ? "" : "disabled") + R"rawliteral(>
                    <div class="field-description">Enter the port for the Loki server.</div>
                </div>
                <script>
                    document.getElementById('loki_enabled').addEventListener('change', function() {
                        var fields = document.getElementById('lokiFields');
                        var inputs = fields.querySelectorAll('input');
                        if (this.checked) {
                            fields.classList.remove('disabled');
                            for (var i = 0; i < inputs.length; i++) inputs[i].disabled = false;
                        } else {
                            fields.classList.add('disabled');
                            for (var i = 0; i < inputs.length; i++) inputs[i].disabled = true;
                        }
                    });
                </script>
            </div>
            <div class="update-section">
                <div>Auto Update:</div>
                <input type="hidden" name="auto_update" value="0">
                <div class="checkbox-row">
                    <input type="checkbox" id="auto_update" name="auto_update" value="1")rawliteral" + (autoUpdate ? " checked" : "") + R"rawliteral(>
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
    bool autoUpdate = false;
    bool lokiEnabled = false;
    for (int i = 0; i < server.args(); i++) {
      if (server.argName(i) == "auto_update" && server.arg(i) == "1") {
        autoUpdate = true;
      }
      if (server.argName(i) == "loki_enabled" && server.arg(i) == "1") {
        lokiEnabled = true;
      }
    }

    Serial.printf("Form received - auto_update: %s, loki_enabled: %s\n", autoUpdate ? "true" : "false", lokiEnabled ? "true" : "false");

    saveWiFiCredentials(ssid, password);
    saveLocationData(latitude, longitude);
    saveBrightness(brightness);
    saveDisplayMode(displayMode);
    saveTimeDisplayDuration(timeDisplayDuration);
    saveTempDisplayDuration(tempDisplayDuration);
    saveAutoUpdate(autoUpdate);
    saveLokiEnabled(lokiEnabled);

    if (lokiEnabled) {
      writeStringToEEPROM(LOKI_IP1_ADDR, lokiIP1, 3);
      writeStringToEEPROM(LOKI_IP2_ADDR, lokiIP2, 3);
      writeStringToEEPROM(LOKI_IP3_ADDR, lokiIP3, 3);
      writeStringToEEPROM(LOKI_IP4_ADDR, lokiIP4, 3);
      writeStringToEEPROM(LOKI_PORT_ADDR, lokiPort, 5);
    }
    EEPROM.commit();

    server.send(200, "text/html", "<h1>Configuration Saved!</h1><p>Rebooting...</p>");
    loki("config", "Configuration saved, rebooting");
    Serial.println("Rebooting now...");
    Serial.flush();
    delay(500);
    ESP.restart();
  } else {
    server.send(405, "text/plain", "Method Not Allowed");
  }
}

// Serve metrics for Prometheus
void serveMetrics() {
  getLocalTime(&timeinfo);
  loki("metrics", "/metrics endpoint scraped");
  String metrics = buildMetricsString(
      uptime_seconds,
      system_get_free_heap_size(),
      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
      localtemp, brightness, displayMode,
      timeDisplayDuration, tempDisplayDuration
  );
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
      lastWifiConnectedAt = millis();
      wifiWasEverConnected = true;
      wifiDisconnectedSince = 0;
      return true;
    }
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nFailed to connect.");
  if (wifiDisconnectedSince == 0) wifiDisconnectedSince = millis();
  return false;
}

// Fetch temperature and timezone from Open-Meteo API
bool fetchTemperatureAndTimezone(const String &latitude, const String &longitude, float &temperature, long &gmtOffsetSec, int &daylightOffsetSec) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClient client;
    String url = buildOpenMeteoUrl(latitude, longitude);
    Serial.print("Request URL: ");
    Serial.println(url);
    loki("weatherserver", "Request: " + url);
    http.begin(client, url);
    Serial.println("http.begin - ok");
    int httpCode = http.GET();
    Serial.println("GET - ok");
    Serial.print("Http response code: ");
    Serial.println(httpCode);
    loki("weatherserver", "HTTP response code: " + String(httpCode));
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.print("Response: ");
      Serial.println(payload);
      loki("weatherserver", "Response: " + payload);
      
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
      loki("weatherserver", "HTTP error: " + String(httpCode));
    }
    http.end();
  }
  reconnectWifi();
  return false;
}

// Update local temperature and timezone from the internet
void updateLocalTemperatureAndTimezone() {
  if (apMode) return;
  String latitude, longitude;
  loadLocationData(latitude, longitude);
  if (fetchTemperatureAndTimezone(latitude, longitude, localtemp, gmtOffsetSec, daylightOffsetSec)) {
    Serial.print("Temperature: ");
    Serial.println(localtemp);
    Serial.print("GMT Offset: ");
    Serial.println(gmtOffsetSec);
    Serial.print("Daylight Saving Offset: ");
    Serial.println(daylightOffsetSec);
    loki("weatherserver", "Temperature fetched: " + String(localtemp) + "C");
    configTime(gmtOffsetSec, daylightOffsetSec, ntpServer1, ntpServer2);
    delay(100);
    if (getLocalTime(&timeinfo)) {
      if (!ntpSyncEnabled) {
        ntpSyncEnabled = true;
        char timeStr[30];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
        loki("system", "NTP: " + String(timeStr) + " | Firmware: " + String(FIRMWARE_VERSION) + " | WiFi: " + ssid + ", IP: " + WiFi.localIP().toString() + ", MAC: " + WiFi.macAddress());
      }
    }
  } else {
    Serial.println("Failed to fetch temperature and timezone.");
    loki("weatherserver", "Failed to fetch temperature and timezone.");
  }
}

// Refresh display
void refreshDisplay() {
  uptime_seconds++;
  getLocalTime(&timeinfo);

  auto decision = decideDisplayContent(
    apMode,
    wifiWasEverConnected,
    wifiDisconnectedSince,
    millis(),
    displayMode,
    showTime,
    timeinfo.tm_hour,
    timeinfo.tm_min,
    localtemp
  );

  if (lastDisplayContent != String(decision.text)) {
    loki("display", "change: " + String(decision.text));
    lastDisplayContent = decision.text;
  }

  matrixDisplay.setTextAlignment(decision.centered ? PA_CENTER : PA_RIGHT);
  matrixDisplay.setFont(BigFontNew);
  matrixDisplay.setIntensity(brightness);
  matrixDisplay.print(decision.text);
}

// Display time
void displayTime() {
  showTime = true;
  refreshDisplay();
  t.setTimeout(displayTemperature, timeDisplayDuration * 1000);
}

// Display temperature
void displayTemperature() {
  showTime = false;
  refreshDisplay();
  t.setTimeout(displayTime, tempDisplayDuration * 1000);
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
  Serial.println("Uptime: " + String(uptime_seconds) + "s, Free memory: " + String(system_get_free_heap_size()) + " bytes");
  loki("heartbeat", "Uptime: " + String(uptime_seconds) + "s, Free memory: " + String(system_get_free_heap_size()) + " bytes");
}


void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);

  // Initialize Loki configuration
  initLokiConfig();

  Serial.printf("FIRMWARE VERSION: %s\n", FIRMWARE_VERSION);

  // Get the MAC address
  String macAddress = WiFi.macAddress();
  Serial.print("MAC Address: ");
  Serial.println(macAddress);
  deviceName = generateDeviceName(macAddress);

  // Initialize LED Matrix display
  matrixDisplay.begin();
  matrixDisplay.setCharSpacing(0);
  matrixDisplay.displayClear();

  // Load configuration from EEPROM
  brightness = loadBrightness();
  displayMode = loadDisplayMode();
  timeDisplayDuration = loadTimeDisplayDuration();
  tempDisplayDuration = loadTempDisplayDuration();

  matrixDisplay.setIntensity(brightness); // Set initial brightness level

  String password;
  if (loadWiFiCredentials(ssid, password) && connectToWiFi(ssid, password)) {
    Serial.println("WiFi connected using stored credentials.");
    displayIPAddress(); // Display IP address in parts

    handleUpdateBootCheck();

    bool autoUpdate = loadAutoUpdate();
    if (autoUpdate) {
      String latestTag, downloadUrl;
      if (checkForUpdates(latestTag, downloadUrl)) {
        Serial.printf("Update available: %s\n", latestTag.c_str());
        EEPROM.write(UPDATE_PENDING_ADDR, 1);
        EEPROM.commit();

        matrixDisplay.setTextAlignment(PA_CENTER);
        matrixDisplay.setFont(BigFontNew);
        matrixDisplay.print("+");

        Serial.println("Starting OTA flashing...");
        if (performOTAUpdate(downloadUrl)) {
          ESP.restart();
        } else {
          Serial.println("Update failed, continuing with current firmware");
          matrixDisplay.print("ERR");
          delay(2000);
        }
      } else {
        Serial.println("No updates available, current version: " + String(FIRMWARE_VERSION));
        matrixDisplay.setTextAlignment(PA_CENTER);
        matrixDisplay.setFont(BigFontNew);
        matrixDisplay.print("-");
        delay(1000);
      }
    }
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

  Serial.println("setup() completed.");
}

void loop() {
  server.handleClient();
  matrixDisplay.displayAnimate();

  if (WiFi.getMode() != WIFI_AP) {
    if (WiFi.status() != WL_CONNECTED) {
      if (wifiDisconnectedSince == 0) {
        wifiDisconnectedSince = millis();
        Serial.println("WiFi lost");
        loki("wifi", "WiFi connection lost");
      }
    } else {
      if (wifiDisconnectedSince > 0) {
        Serial.printf("WiFi reconnected after %lus\n", (millis() - wifiDisconnectedSince) / 1000);
        loki("wifi", "WiFi reconnected after " + String((millis() - wifiDisconnectedSince) / 1000) + "s");
        wifiDisconnectedSince = 0;
        lastWifiConnectedAt = millis();
        updateLocalTemperatureAndTimezone();
      }
    }
  }

  t.handle();
}
 