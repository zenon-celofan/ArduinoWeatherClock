#pragma once

#include <WString.h>

String generateDeviceName(const String &macAddress);

String buildHeartbeatMessage(unsigned long uptimeSeconds, unsigned long freeHeap);

String formatIPOctet(int octet, bool addDot);

String generateAPName(const String &macAddress);
