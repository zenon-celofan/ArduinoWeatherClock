#pragma once

#include <WString.h>
#include <cstdint>

enum wl_status_t {
    WL_NO_SHIELD = 255,
    WL_IDLE_STATUS = 0,
    WL_NO_SSID_AVAIL = 1,
    WL_SCAN_COMPLETED = 2,
    WL_CONNECTED = 3,
    WL_CONNECT_FAILED = 4,
    WL_CONNECTION_LOST = 5,
    WL_DISCONNECTED = 6
};

#define WIFI_STA 1
#define WIFI_AP  2
#define WIFI_AP_STA 3

class IPAddress {
    uint8_t addr_[4];
public:
    IPAddress() : addr_{0,0,0,0} {}
    IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) : addr_{a,b,c,d} {}

    uint8_t operator[](int i) const { return addr_[i]; }
    uint8_t &operator[](int i) { return addr_[i]; }

    String toString() const {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d.%d.%d.%d", addr_[0], addr_[1], addr_[2], addr_[3]);
        return String(buf);
    }
};

class WiFiClass {
    int mode_ = 0;
    wl_status_t status_ = WL_DISCONNECTED;
    int connect_call_count_ = 0;
    int status_call_count_ = 0;
    int connect_after_calls_ = -1;
    IPAddress ip_{192,168,1,100};

public:
    void mode(int m) { mode_ = m; }
    int getMode() const { return mode_; }

    void begin(const char*, const char*) { connect_call_count_++; }

    wl_status_t status() {
        status_call_count_++;
        if (connect_after_calls_ >= 0 && status_call_count_ >= connect_after_calls_) {
            return WL_CONNECTED;
        }
        return status_;
    }
    void setStatus(wl_status_t s) { status_ = s; }
    void setConnectAfterCalls(int n) { connect_after_calls_ = n; }
    void resetCounters() { connect_call_count_ = 0; status_call_count_ = 0; connect_after_calls_ = -1; }

    IPAddress localIP() const { return ip_; }
    void setLocalIP(const IPAddress &ip) { ip_ = ip; }

    int connectCallCount() const { return connect_call_count_; }
    void resetConnectCallCount() { connect_call_count_ = 0; }
};

extern WiFiClass WiFi;
