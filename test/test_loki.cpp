#include "loki_utils.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

#define RUN_TEST(name, expr) do {                                    \
    printf("  TEST: %s ... ", name);                                 \
    if (!(expr)) { printf("FAIL\n"); return 1; }                     \
    printf("PASS\n");                                                \
} while(0)

int main() {
    puts("\n=== buildLokiPayload Tests ===\n");

    String result;

    // --- Basic payload structure ---
    {
        result = buildLokiPayload("sensor-01", "test", "hello", 1234567890ULL);
        const char *r = result.c_str();
        RUN_TEST("contains device", strstr(r, "\"device\":\"sensor-01\"") != NULL);
        RUN_TEST("contains category", strstr(r, "\"category\":\"test\"") != NULL);
        RUN_TEST("contains level info", strstr(r, "\"level\":\"info\"") != NULL);
        RUN_TEST("contains message", strstr(r, "\"hello\"") != NULL);
        RUN_TEST("contains timestamp", strstr(r, "1234567890") != NULL);
        RUN_TEST("starts with {", r[0] == '{');
        RUN_TEST("ends with }", r[result.length() - 1] == '}');
    }

    // --- Device name with spaces and special chars ---
    {
        result = buildLokiPayload("my device-2", "system", "test msg", 0ULL);
        RUN_TEST("device with spaces", strstr(result.c_str(), "\"device\":\"my device-2\"") != NULL);
    }

    // --- Category with underscore ---
    {
        result = buildLokiPayload("dev", "wifi_status", "msg", 0ULL);
        RUN_TEST("category underscore", strstr(result.c_str(), "\"category\":\"wifi_status\"") != NULL);
    }

    // --- Message with special characters ---
    {
        result = buildLokiPayload("dev", "cat", "line1\nline2\t\"quoted\"", 0ULL);
        RUN_TEST("message special chars", strstr(result.c_str(), "line1") != NULL);
        RUN_TEST("tab in message", strstr(result.c_str(), "line2") != NULL);
        RUN_TEST("quotes escaped", strstr(result.c_str(), "\\\"quoted\\\"") != NULL);
    }

    // --- Large timestamp ---
    {
        result = buildLokiPayload("dev", "cat", "msg", 1700000000000000000ULL);
        RUN_TEST("large timestamp", strstr(result.c_str(), "1700000000000000000") != NULL);
    }

    // --- isTimestampValid tests ---
    {
        RUN_TEST("timestamp before 2025 invalid", isTimestampValid(1000000) == false);
        RUN_TEST("timestamp at boundary valid", isTimestampValid(1735689600ULL) == true);
        RUN_TEST("timestamp after boundary valid", isTimestampValid(1800000000ULL) == true);
        RUN_TEST("timestamp zero invalid", isTimestampValid(0) == false);
        RUN_TEST("timestamp 2024 invalid", isTimestampValid(1700000000ULL) == false);
    }

    puts("\n---\nAll loki tests passed!");
    return 0;
}
