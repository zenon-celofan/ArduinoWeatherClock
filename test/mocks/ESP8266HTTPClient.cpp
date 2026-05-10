#include "ESP8266HTTPClient.h"

int HTTPClient::s_httpCode = 200;
String HTTPClient::s_payload;
bool HTTPClient::s_connected = true;
WiFiClient *HTTPClient::s_streamPtr = nullptr;
