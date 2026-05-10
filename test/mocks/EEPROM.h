#pragma once

#include <cstdint>
#include <cstring>

#ifndef TEST_EEPROM_SIZE
#define TEST_EEPROM_SIZE 128
#endif

namespace {
    uint8_t eeprom_mem[TEST_EEPROM_SIZE];
}

struct EEPROMClass {
    void begin(int size) {
        if (size > TEST_EEPROM_SIZE) size = TEST_EEPROM_SIZE;
        memset(eeprom_mem, 0, size);
    }

    uint8_t read(int addr) {
        if (addr >= 0 && addr < TEST_EEPROM_SIZE) {
            return eeprom_mem[addr];
        }
        return 0;
    }

    void write(int addr, uint8_t val) {
        if (addr >= 0 && addr < TEST_EEPROM_SIZE) {
            eeprom_mem[addr] = val;
        }
    }

    bool commit() { return true; }
};

static EEPROMClass EEPROM;
