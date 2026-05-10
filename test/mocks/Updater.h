#pragma once

#include <cstdint>
#include <cstdio>

#define U_FLASH 0

class UpdaterClass {
    bool begin_result_ = true;
    size_t write_count_ = 0;
    size_t written_ = 0;
    bool end_result_ = true;
    const char *error_ = "mock error";

public:
    void setBeginResult(bool r) { begin_result_ = r; }
    void setEndResult(bool r) { end_result_ = r; }
    void setError(const char *e) { error_ = e; }
    size_t written() const { return written_; }
    size_t writeCount() const { return write_count_; }

    bool begin(size_t, int) { return begin_result_; }
    size_t write(const uint8_t *, size_t len) {
        write_count_++;
        written_ += len;
        return len;
    }
    bool end(bool) { return end_result_; }
    const char *getErrorString() { return error_; }
};
