#pragma once

#include <WString.h>

bool isTimestampValid(unsigned long long epochSec);

String buildLokiPayload(const String &deviceName, const String &category, const String &logMessage, unsigned long long epochNanoseconds);

String buildLokiUrlFromEEPROM();

bool sendLokiLog(const String &lokiURL, const String &deviceName,
                 const String &category, const String &logMessage);
