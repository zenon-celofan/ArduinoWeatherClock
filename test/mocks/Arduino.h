#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdarg>

class IPAddress;
class String;

extern unsigned long mock_millis;

inline unsigned long millis() { return mock_millis; }

inline void delay(unsigned long) {}
inline void yield() {}

class SerialClass {
public:
    void begin(int) {}
    void print(const char *s) { if (s) fputs(s, stdout); }
    void print(const String &s);
    void print(int v) { fprintf(stdout, "%d", v); }
    void print(unsigned long v) { fprintf(stdout, "%lu", v); }
    void print(float v) { fprintf(stdout, "%f", v); }
    void print(double v) { fprintf(stdout, "%f", v); }
    void println() { fputc('\n', stdout); }
    void println(const char *s) { if (s) puts(s); else putchar('\n'); }
    void println(int v) { fprintf(stdout, "%d\n", v); }
    void println(unsigned long v) { fprintf(stdout, "%lu\n", v); }
    void println(float v) { fprintf(stdout, "%f\n", v); }
    void println(double v) { fprintf(stdout, "%f\n", v); }
    void println(const String &s);
    void println(const IPAddress &ip);
    void printf(const char *fmt, ...) {
        va_list args;
        va_start(args, fmt);
        vfprintf(stdout, fmt, args);
        va_end(args);
    }
    void flush() { fflush(stdout); }
};

extern SerialClass Serial;
