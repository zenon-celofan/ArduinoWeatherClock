#pragma once

#include <cstdint>
#include <cstring>

#define TEST_EEPROM_SIZE 128

extern uint8_t test_eeprom_mem[TEST_EEPROM_SIZE];

struct EEPROMClass {
    void begin(int size) {
        if (size > TEST_EEPROM_SIZE) size = TEST_EEPROM_SIZE;
        memset(test_eeprom_mem, 0, size);
    }

    uint8_t read(int addr) {
        if (addr >= 0 && addr < TEST_EEPROM_SIZE) {
            return test_eeprom_mem[addr];
        }
        return 0;
    }

    void write(int addr, uint8_t val) {
        if (addr >= 0 && addr < TEST_EEPROM_SIZE) {
            test_eeprom_mem[addr] = val;
        }
    }

    bool commit() { return true; }
    void reset() { memset(test_eeprom_mem, 0, TEST_EEPROM_SIZE); }
};

extern EEPROMClass EEPROM;
