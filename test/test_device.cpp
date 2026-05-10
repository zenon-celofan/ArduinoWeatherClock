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

    puts("\n=== buildHeartbeatMessage Tests ===\n");

    {
        String msg = buildHeartbeatMessage(0, 0);
        RUN_TEST("zero values", msg == "Uptime: 0s, Free memory: 0 bytes");
    }
    {
        String msg = buildHeartbeatMessage(1, 100);
        RUN_TEST("low values", msg == "Uptime: 1s, Free memory: 100 bytes");
    }
    {
        String msg = buildHeartbeatMessage(3600, 4096);
        RUN_TEST("hour uptime", msg == "Uptime: 3600s, Free memory: 4096 bytes");
    }
    {
        String msg = buildHeartbeatMessage(86400, 65535);
        RUN_TEST("day uptime", msg == "Uptime: 86400s, Free memory: 65535 bytes");
    }
    {
        String msg = buildHeartbeatMessage(999999, 123456);
        RUN_TEST("large values", msg == "Uptime: 999999s, Free memory: 123456 bytes");
    }

    puts("\n=== generateAPName Tests ===\n");

    RUN_TEST("full MAC", generateAPName("AA:BB:CC:DD:EE:FF") == "Clock_AP_DD:EE:FF");
    RUN_TEST("different MAC", generateAPName("00:11:22:33:44:55") == "Clock_AP_33:44:55");
    RUN_TEST("short MAC", generateAPName("AB:CD:EF") == "Clock_AP_AB:CD:EF");

    puts("\n=== formatIPOctet Tests ===\n");

    RUN_TEST("192 with dot", formatIPOctet(192, true) == "192.");
    RUN_TEST("168 with dot", formatIPOctet(168, true) == "168.");
    RUN_TEST("1 with dot", formatIPOctet(1, true) == "001.");
    RUN_TEST("0 with dot", formatIPOctet(0, true) == "000.");
    RUN_TEST("255 with dot", formatIPOctet(255, true) == "255.");
    RUN_TEST("10 without dot", formatIPOctet(10, false) == "010");
    RUN_TEST("0 without dot", formatIPOctet(0, false) == "000");
    RUN_TEST("255 without dot", formatIPOctet(255, false) == "255");
    RUN_TEST("last octet no dot", formatIPOctet(100, false) == "100");

    puts("\n---\nAll device name tests passed!");
    return 0;
}
