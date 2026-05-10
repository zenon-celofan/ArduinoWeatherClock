#pragma once

#include <string>
#include <cstdio>
#include <cstring>
#include <cstdlib>

class String {
  std::string s_;

public:
  String() {}
  String(const char *cstr) : s_(cstr ? cstr : "") {}
  String(const String &other) : s_(other.s_) {}
  String(int val) : s_(std::to_string(val)) {}
  String(unsigned int val) : s_(std::to_string(val)) {}
  String(long val) : s_(std::to_string(val)) {}
  String(unsigned long val) : s_(std::to_string(val)) {}

  const char *c_str() const { return s_.c_str(); }
  int length() const { return s_.length(); }

  void replace(const char *from, const char *to) {
    size_t fromLen = strlen(from);
    size_t toLen = strlen(to);
    size_t pos = 0;
    while ((pos = s_.find(from, pos)) != std::string::npos) {
      s_.replace(pos, fromLen, to);
      pos += toLen;
    }
  }

  char operator[](int i) const { return s_[i]; }
  char &operator[](int i) { return s_[i]; }

  String &operator=(const String &other) {
    s_ = other.s_;
    return *this;
  }
  String &operator=(const char *cstr) {
    s_ = cstr ? cstr : "";
    return *this;
  }

  friend String operator+(const String &a, const String &b) {
    return String((a.s_ + b.s_).c_str());
  }
  friend String operator+(const String &a, const char *b) {
    return String((a.s_ + b).c_str());
  }
  friend String operator+(const char *a, const String &b) {
    return String((std::string(a) + b.s_).c_str());
  }

  String substring(int from) const {
    return String(s_.substr(from).c_str());
  }
  String substring(int from, int to) const {
    return String(s_.substr(from, to - from).c_str());
  }

  bool operator==(const String &other) const { return s_ == other.s_; }
  bool operator!=(const String &other) const { return s_ != other.s_; }
  bool operator<(const String &other) const { return s_ < other.s_; }
  bool operator>(const String &other) const { return s_ > other.s_; }
};
