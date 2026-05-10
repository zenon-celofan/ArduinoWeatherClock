#include "semver_compare.h"
#include <cstdio>

int semverCompare(const String &v1, const String &v2) {
  String s1 = v1, s2 = v2;
  s1.replace("v", ""); s2.replace("v", "");

  int major1 = 0, minor1 = 0, patch1 = 0;
  int major2 = 0, minor2 = 0, patch2 = 0;

  sscanf(s1.c_str(), "%d.%d.%d", &major1, &minor1, &patch1);
  sscanf(s2.c_str(), "%d.%d.%d", &major2, &minor2, &patch2);

  if (major1 > major2) return 1;
  if (major1 < major2) return -1;
  if (minor1 > minor2) return 1;
  if (minor1 < minor2) return -1;
  if (patch1 > patch2) return 1;
  if (patch1 < patch2) return -1;
  return 0;
}
