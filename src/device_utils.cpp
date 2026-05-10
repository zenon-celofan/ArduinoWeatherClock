#include "device_utils.h"

String generateDeviceName(const String &macAddress) {
  String name = "device-" + macAddress.substring(macAddress.length() - 5);
  name.replace(":", "");
  return name;
}
