#include "Arduino.h"
#include "ESP8266WiFi.h"

unsigned long mock_millis = 0;
SerialClass Serial;

void SerialClass::print(const String &s) {
    fputs(s.c_str(), stdout);
}

void SerialClass::println(const String &s) {
    puts(s.c_str());
}

void SerialClass::println(const IPAddress &ip) {
    fprintf(stdout, "%d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
}
