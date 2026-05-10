#pragma once

#include <WString.h>

String buildMetricsString(
    unsigned long uptimeSeconds,
    int freeHeapBytes,
    int hour, int minute, int second,
    float localtemp,
    int brightness,
    int displayMode,
    int timeDisplayDuration,
    int tempDisplayDuration
);
