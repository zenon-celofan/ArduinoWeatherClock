#pragma once

#include <WString.h>
#include <cstdint>
#include <cstring>

#define HTTP_CODE_OK 200

class WiFiClient {
    size_t remaining_ = 0;
    size_t chunk_size_ = 512;
public:
    void setStreamBytes(size_t total) { remaining_ = total; }
    void setChunkSize(size_t c) { chunk_size_ = c; }
    int available() {
        if (remaining_ == 0) return 0;
        int a = (remaining_ < chunk_size_) ? (int)remaining_ : (int)chunk_size_;
        return a;
    }
    size_t readBytes(uint8_t *buf, size_t len) {
        size_t to_read = (len < remaining_) ? len : remaining_;
        for (size_t i = 0; i < to_read; i++) buf[i] = 0xAA;
        remaining_ -= to_read;
        return to_read;
    }
};

class HTTPClient {
    static int s_httpCode;
    static String s_payload;
    static bool s_connected;
    static WiFiClient *s_streamPtr;

public:
    static void setHttpCode(int code) { s_httpCode = code; }
    static void setPayload(const String &p) { s_payload = p; }
    static void setConnected(bool c) { s_connected = c; }
    static void setStreamPtr(WiFiClient *p) { s_streamPtr = p; }

    void begin(WiFiClient &, const String &) {}
    void addHeader(const String &, const String &) {}
    int GET() { return s_httpCode; }
    int POST(const String &) { return s_httpCode; }
    String getString() { return s_payload; }
    void end() {}
    void setFollowRedirects(bool) {}
    void setTimeout(unsigned long) {}
    int getSize() { return s_payload.length(); }
    bool connected() { return s_connected; }
    WiFiClient *getStreamPtr() { return s_streamPtr; }
};
