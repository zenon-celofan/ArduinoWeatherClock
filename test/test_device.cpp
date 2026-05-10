#include "device_utils.h"
#include <cstdio>
#include <cstdlib>

#define RUN_TEST(name, expr) do {                                    \
    printf("  TEST: %s ... ", name);                                 \
    if (!(expr)) { printf("FAIL\n"); return 1; }                     \
    printf("PASS\n");                                                \
} while(0)

int main() {
    puts("\n=== generateDeviceName Tests ===\n");

    RUN_TEST("standard MAC",
        generateDeviceName("AA:BB:CC:DD:EE:FF") == "device-EEFF");

    RUN_TEST("different suffix",
        generateDeviceName("00:11:22:33:44:55") == "device-4455");

    RUN_TEST("colons stripped",
        generateDeviceName("AB:CD:EF:01:23:45") == "device-2345");

    RUN_TEST("only last 5 chars of MAC used",
        generateDeviceName("XX:YY:ZZ:AB:CD:EF") == "device-CDEF");

    RUN_TEST("numeric chars",
        generateDeviceName("12:34:56:78:90:AB") == "device-90AB");

    // Edge: short string (unlikely for real MAC, but test robustness)
    RUN_TEST("short input",
        generateDeviceName("A:B:C") == "device-ABC");

    puts("\n---\nAll device name tests passed!");
    return 0;
}
