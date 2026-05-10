#include "eeprom_utils.h"
#include <EEPROM.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#define RUN_TEST(name, expr) do {                                    \
    printf("  TEST: %s ... ", name);                                 \
    if (!(expr)) { printf("FAIL\n"); return 1; }                     \
    printf("PASS\n");                                                \
} while(0)

static void wipe_eeprom() {
    for (int i = 0; i < 128; i++) EEPROM.write(i, 0);
    EEPROM.commit();
}

int main() {
    puts("\n=== EEPROM String Utils Tests ===\n");

    EEPROM.begin(128);

    // Basic write + read roundtrip
    wipe_eeprom();
    writeStringToEEPROM(0, "hello", 10);
    String result = readStringFromEEPROM(0, 10);
    RUN_TEST("basic write/read roundtrip", result == "hello");

    // Verify trailing bytes are zeroed
    wipe_eeprom();
    writeStringToEEPROM(0, "ab", 5);
    result = readStringFromEEPROM(0, 5);
    RUN_TEST("trailing bytes zeroed", result == "ab" && result.length() == 2);

    // Truncation: string longer than maxLength
    wipe_eeprom();
    writeStringToEEPROM(0, "toolong", 3);
    result = readStringFromEEPROM(0, 3);
    RUN_TEST("truncation to maxLength", result == "too" && result.length() == 3);

    // Empty string
    wipe_eeprom();
    writeStringToEEPROM(0, "", 10);
    result = readStringFromEEPROM(0, 10);
    RUN_TEST("empty string", result == "" && result.length() == 0);

    // Non-zero start address
    wipe_eeprom();
    writeStringToEEPROM(50, "world", 10);
    result = readStringFromEEPROM(50, 10);
    RUN_TEST("non-zero start address", result == "world");

    // Multiple strings don't overlap
    wipe_eeprom();
    writeStringToEEPROM(0, "foo", 5);
    writeStringToEEPROM(10, "bar", 5);
    String r1 = readStringFromEEPROM(0, 5);
    String r2 = readStringFromEEPROM(10, 5);
    RUN_TEST("multiple strings no overlap", r1 == "foo" && r2 == "bar");

    // String with special characters
    wipe_eeprom();
    writeStringToEEPROM(0, "a b\tc", 10);
    result = readStringFromEEPROM(0, 10);
    RUN_TEST("string with spaces/tabs", result == "a b\tc");

    // Verify EEPROM is actually written at correct addresses
    wipe_eeprom();
    writeStringToEEPROM(5, "hi", 10);
    RUN_TEST("byte 5 = 'h'", EEPROM.read(5) == 'h');
    RUN_TEST("byte 6 = 'i'", EEPROM.read(6) == 'i');
    RUN_TEST("byte 7 = null", EEPROM.read(7) == 0);

    puts("\n---\nAll EEPROM tests passed!");
    return 0;
}
