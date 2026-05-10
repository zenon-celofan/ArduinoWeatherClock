#include "temp_utils.h"
#include <cmath>
#include <cstdio>

String formatTemperature(float tempCelsius) {
  int temp = round(tempCelsius);
  char buf[8];
  snprintf(buf, sizeof(buf), temp > 0 ? "+%d" : "%d", temp);
  return String(buf);
}
