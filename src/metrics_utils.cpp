#include "metrics_utils.h"

String buildMetricsString(
    unsigned long uptimeSeconds,
    int freeHeapBytes,
    int hour, int minute, int second,
    float localtemp,
    int brightness,
    int displayMode,
    int timeDisplayDuration,
    int tempDisplayDuration
) {
  String out;
  out += "# HELP uptime_seconds The number of seconds the system has been running.\n";
  out += "# TYPE uptime_seconds counter\n";
  out += "uptime_seconds " + String(uptimeSeconds) + "\n";
  out += "\n";
  out += "# HELP free_heap_mem_bytes Free heap memory left.\n";
  out += "# TYPE free_heap_mem_bytes gauge\n";
  out += "free_heap_mem_bytes " + String(freeHeapBytes) + "\n";
  out += "\n";
  out += "# HELP localtime_hours The current hour of the day in 24-hour format.\n";
  out += "# TYPE localtime_hours gauge\n";
  out += "localtime_hours " + String(hour) + "\n";
  out += "\n";
  out += "# HELP localtime_minutes The current minute of the hour.\n";
  out += "# TYPE localtime_minutes gauge\n";
  out += "localtime_minutes " + String(minute) + "\n";
  out += "\n";
  out += "# HELP localtime_seconds The current second of the minute.\n";
  out += "# TYPE localtime_seconds gauge\n";
  out += "localtime_seconds " + String(second) + "\n";
  out += "\n";
  out += "# HELP localtemp The current local temperature in Celsius.\n";
  out += "# TYPE localtemp gauge\n";
  out += "localtemp " + String(localtemp) + "\n";
  out += "\n";
  out += "# HELP brightness The current brightness level of the LED display.\n";
  out += "# TYPE brightness gauge\n";
  out += "brightness " + String(brightness) + "\n";
  out += "\n";
  out += "# HELP display_mode The current display mode of the LED display (0 = both, 1 = time only, 2 = temperature only).\n";
  out += "# TYPE display_mode gauge\n";
  out += "display_mode " + String(displayMode) + "\n";
  out += "\n";
  out += "# HELP time_display_duration The duration to display the time in seconds.\n";
  out += "# TYPE time_display_duration gauge\n";
  out += "time_display_duration " + String(timeDisplayDuration) + "\n";
  out += "\n";
  out += "# HELP temp_display_duration The duration to display the temperature in seconds.\n";
  out += "# TYPE temp_display_duration gauge\n";
  out += "temp_display_duration " + String(tempDisplayDuration) + "\n";
  return out;
}
