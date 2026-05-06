FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    curl \
    python3 \
    git \
    && rm -rf /var/lib/apt/lists/*

RUN curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=/usr/local/bin sh

RUN arduino-cli config init \
    --additional-urls https://arduino.esp8266.com/stable/package_esp8266com_index.json \
    && arduino-cli core update-index \
    && arduino-cli core install esp8266:esp8266 \
    && arduino-cli lib install MD_Parola MD_MAX72XX ArduinoJson AsyncTimer \
    && ln -s /root/Arduino/libraries/MD_MAX72XX/src/MD_MAX72xx.h /root/Arduino/libraries/MD_MAX72XX/src/MD_MAX72XX.h

CMD ["bash"]
