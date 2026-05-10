#pragma once

#include <WString.h>

bool isTimestampValid(unsigned long long epochSec);

String buildLokiPayload(const String &deviceName, const String &category, const String &logMessage, unsigned long long epochNanoseconds);
