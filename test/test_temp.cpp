#include "temp_utils.h"
#include <cstdio>
#include <cstdlib>

#define RUN_TEST(name, expr) do {                                    \
    printf("  TEST: %s ... ", name);                                 \
    if (!(expr)) { printf("FAIL\n"); return 1; }                     \
    printf("PASS\n");                                                \
} while(0)

int main() {
    puts("\n=== formatTemperature Tests ===\n");

    RUN_TEST("positive temp",       formatTemperature(23.0f) == "+23");
    RUN_TEST("positive rounded up", formatTemperature(23.7f) == "+24");

    RUN_TEST("negative temp",       formatTemperature(-5.0f) == "-5");
    RUN_TEST("negative rounded",    formatTemperature(-5.7f) == "-6");

    RUN_TEST("zero",                formatTemperature(0.0f) == "0");
    RUN_TEST("near zero positive",  formatTemperature(0.4f) == "0");
    RUN_TEST("near zero negative",  formatTemperature(-0.4f) == "0");

    RUN_TEST("+0.5 rounds to +1",   formatTemperature(0.5f) == "+1");
    RUN_TEST("-0.5 rounds to -1",   formatTemperature(-0.5f) == "-1");

    RUN_TEST("+10",                 formatTemperature(10.0f) == "+10");
    RUN_TEST("+99",                 formatTemperature(99.0f) == "+99");

    RUN_TEST("-10",                 formatTemperature(-10.0f) == "-10");
    RUN_TEST("-99",                 formatTemperature(-99.0f) == "-99");

    puts("\n---\nAll temp tests passed!");
    return 0;
}
