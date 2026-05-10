#include "device_utils.h"
#include <cstdio>

String generateDeviceName(const String &macAddress) {
  String name = "device-" + macAddress.substring(macAddress.length() - 5);
  name.replace(":", "");
  return name;
}

String buildHeartbeatMessage(unsigned long uptimeSeconds, unsigned long freeHeap) {
  char buf[64];
  snprintf(buf, sizeof(buf), "Uptime: %lus, Free memory: %lu bytes", uptimeSeconds, freeHeap);
  return String(buf);
}

String generateAPName(const String &macAddress) {
  String suffix = macAddress.substring(macAddress.length() - 8);
  return "Clock_AP_" + suffix;
}

String formatIPOctet(int octet, bool addDot) {
  char buf[5];
  snprintf(buf, sizeof(buf), addDot ? "%03d." : "%03d", octet);
  return String(buf);
}
