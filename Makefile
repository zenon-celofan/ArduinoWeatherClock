FQBN      := esp8266:esp8266:d1_mini
SKETCH    := ArduinoWeatherClock.ino
BUILD_DIR := build
ARDUINO_CLI := /home/n/bin/arduino-cli

.PHONY: all build clean test

all: build

build:
	$(ARDUINO_CLI) compile --fqbn $(FQBN) --output-dir $(BUILD_DIR) --build-property "compiler.cpp.extra_flags=-I include" $(SKETCH)

clean:
	rm -f $(BUILD_DIR)/*.bin $(BUILD_DIR)/*.elf $(BUILD_DIR)/*.map

test:
	$(MAKE) -C test test
