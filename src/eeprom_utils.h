#pragma once

#include <WString.h>

String readStringFromEEPROM(int startAddr, int length);
void writeStringToEEPROM(int startAddr, const String &str, int maxLength);
