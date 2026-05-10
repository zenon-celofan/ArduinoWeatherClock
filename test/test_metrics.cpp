#include "metrics_utils.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define RUN_TEST(name, expr) do {                                    \
    printf("  TEST: %s ... ", name);                                 \
    if (!(expr)) { printf("FAIL\n"); return 1; }                     \
    printf("PASS\n");                                                \
} while(0)

static bool contains(const String &s, const char *sub) {
    return strstr(s.c_str(), sub) != nullptr;
}

int main() {
    puts("\n=== Metrics Utils Tests ===\n");

    String m = buildMetricsString(
        12345,           // uptimeSeconds
        40960,           // freeHeapBytes
        14, 30, 5,       // hour, minute, second
        23.5f,           // localtemp
        7,               // brightness
        0,               // displayMode
        5,               // timeDisplayDuration
        10               // tempDisplayDuration
    );

    // --- Prometheus format basics ---
    RUN_TEST("contains # HELP for uptime", contains(m, "# HELP uptime_seconds"));
    RUN_TEST("contains # TYPE uptime", contains(m, "# TYPE uptime_seconds counter"));
    RUN_TEST("contains uptime value", contains(m, "uptime_seconds 12345"));

    RUN_TEST("contains # HELP free_heap", contains(m, "# HELP free_heap_mem_bytes"));
    RUN_TEST("contains # TYPE free_heap", contains(m, "# TYPE free_heap_mem_bytes gauge"));
    RUN_TEST("contains free_heap value", contains(m, "free_heap_mem_bytes 40960"));

    RUN_TEST("contains hour value", contains(m, "localtime_hours 14"));
    RUN_TEST("contains minute value", contains(m, "localtime_minutes 30"));
    RUN_TEST("contains second value", contains(m, "localtime_seconds 5"));

    RUN_TEST("contains localtemp value", contains(m, "localtemp 23.50"));
    RUN_TEST("contains brightness value", contains(m, "brightness 7"));
    RUN_TEST("contains displayMode value", contains(m, "display_mode 0"));
    RUN_TEST("contains timeDisplayDuration", contains(m, "time_display_duration 5"));
    RUN_TEST("contains tempDisplayDuration", contains(m, "temp_display_duration 10"));

    // --- Edge cases ---
    String m2 = buildMetricsString(0, 0, 0, 0, 0, -5.0f, 0, 2, 1, 60);
    RUN_TEST("zero uptime", contains(m2, "uptime_seconds 0"));
    RUN_TEST("midnight hour", contains(m2, "localtime_hours 0"));
    RUN_TEST("midnight minute", contains(m2, "localtime_minutes 0"));
    RUN_TEST("midnight second", contains(m2, "localtime_seconds 0"));
    RUN_TEST("negative temp", contains(m2, "localtemp -5.00"));

    // --- Verify all 9 metrics are present ---
    int helpCount = 0;
    const char *p = m.c_str();
    while ((p = strstr(p, "# HELP ")) != nullptr) { helpCount++; p++; }
    RUN_TEST("10 HELP lines", helpCount == 10);

    puts("\n---\nAll metrics tests passed!");
    return 0;
}
