#include "eeprom_utils.h"
#include <EEPROM.h>

String readStringFromEEPROM(int startAddr, int length) {
  char data[length + 1];
  for (int i = 0; i < length; i++) {
    data[i] = EEPROM.read(startAddr + i);
  }
  data[length] = '\0';
  return String(data);
}

void writeStringToEEPROM(int startAddr, const String &str, int maxLength) {
  for (int i = 0; i < maxLength; i++) {
    if (i < str.length()) {
      EEPROM.write(startAddr + i, str[i]);
    } else {
      EEPROM.write(startAddr + i, 0);
    }
  }
}
