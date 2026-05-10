CXX      ?= g++
CXXFLAGS ?= -std=c++11 -Wall -Wextra -Werror -I ../src -I mocks

SRC      := ../src
MOCKS    := mocks
JSON_INC := /home/n/Arduino/libraries/ArduinoJson/src
TESTS    := test_semver test_eeprom test_temp test_url test_device test_eeprom_config test_metrics test_display test_update test_loki test_weather

.PHONY: all clean test

all: $(TESTS)

test_semver: test_semver.cpp $(SRC)/semver_compare.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test_eeprom: test_eeprom.cpp $(SRC)/eeprom_utils.cpp $(MOCKS)/EEPROM.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test_temp: test_temp.cpp $(SRC)/temp_utils.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test_url: test_url.cpp $(SRC)/url_utils.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test_device: test_device.cpp $(SRC)/device_utils.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test_eeprom_config: test_eeprom_config.cpp $(SRC)/eeprom_config.cpp $(SRC)/eeprom_utils.cpp $(MOCKS)/EEPROM.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test_metrics: test_metrics.cpp $(SRC)/metrics_utils.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test_display: test_display.cpp $(SRC)/display_utils.cpp $(SRC)/temp_utils.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test_update: test_update.cpp $(SRC)/update_utils.cpp $(SRC)/semver_compare.cpp
	$(CXX) $(CXXFLAGS) -I $(JSON_INC) -o $@ $^

test_loki: test_loki.cpp $(SRC)/loki_utils.cpp
	$(CXX) $(CXXFLAGS) -I $(JSON_INC) -o $@ $^

test_weather: test_weather.cpp $(SRC)/weather_utils.cpp
	$(CXX) $(CXXFLAGS) -I $(JSON_INC) -o $@ $^

test: $(TESTS)
	@passed=0; failed=0; \
	for t in $(TESTS); do \
		echo ""; \
		echo "Running $$t..."; \
		./$$t && passed=$$((passed+1)) || failed=$$((failed+1)); \
	done; \
	echo ""; \
	echo "=== Results: $$passed passed, $$failed failed ==="; \
	[ $$failed -eq 0 ]

clean:
	rm -f $(TESTS)
