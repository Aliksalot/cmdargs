#pragma once

#include"./darray.h"
#include<cstddef>
#include<stdexcept>
#include<cstring>
#include<ostream>
#include<istream>

namespace clib{

  class Text {
  public:
    Text();
    Text(const char* s);
    Text(const char* s, std::size_t len);

    const char& operator[](std::size_t index) const;
    char& operator[](std::size_t index);
    Text& operator+=(const Text& s);
    Text& operator+=(char c);
    Text& operator+=(const char* s);
    Text& operator=(const char* s);
    bool operator==(const Text& s) const;
    bool operator==(const char* s) const;
    bool operator!=(const Text& s) const;
    bool operator!=(const char* s) const;

    long long toInt() const;
    static Text fromInt(long long i);
    bool isValidInt() const;

    Text& insert(std::size_t at, char c);
    Text trim(char c = ' ') const;
    Text& trimInPlace(char c = ' ');

    darray<Text> split(char c) const;

    bool empty() const;

    void clear();
    std::size_t size() const;

    const char* raw() const;

  private:
    darray<char> l;
  };

  inline Text& Text::trimInPlace(char c) {
    l.pop();

    while(!l.empty() && l[0] == c) {
      l.remove(0);
    }

    while(!l.empty() && l[l.size() - 1] == c) {
      l.remove(l.size() - 1);
    }

    l.add('\0');
    return *this;
  }

  inline Text Text::trim(char c) const {
    Text trimmed(*this);
    return trimmed.trimInPlace(c);
  }

  inline void Text::clear() {
    l.clear();
    l.add('\0');
  }
  inline std::size_t Text::size() const {
    return l.size() - 1;
  }

  inline Text::Text() { l.add('\0'); };
  inline Text::Text(const char* s): Text(s, std::strlen(s)) { };
  inline Text::Text(const char* s, std::size_t len) {
    l = darray<char>(s, len);
    l.add('\0');
  }

  inline bool Text::empty() const {
    return size() == 0;
  }

  inline Text& Text::operator=(const char* s) {
    l = darray<char>(s, std::strlen(s));
    l.add('\0');
    return *this;
  }
  inline char& Text::operator[](std::size_t index) {
    return l[index];
  }

  inline const char& Text::operator[](std::size_t index) const{
    return l[index];
  }
  inline Text& Text::operator+=(const Text& s) {
    if(this == &s) {
      Text copy = s;
      l.pop();
      l.extend(copy.l);
      return *this;
    }
    l.pop();
    l.extend(s.l);
    return *this;
  }
  inline Text& Text::operator+=(char c) {
    l.pop();
    l.add(c);
    l.add('\0');
    return *this;
  }
  inline Text& Text::operator+=(const char* s) {
    return *this += Text(s);
  }
  inline Text operator+(Text a, const Text& b) {
    return a += b;
  }
  inline Text operator+(Text a, long long n) {
    return a += Text::fromInt(n);
  }
  inline bool Text::operator==(const Text& s) const {
    if(size() != s.size()) return false;

    for(std::size_t i = 0; i < size(); i ++) {
      if(l[i] != s[i]) return false;
    }
    return true;
  }
  inline bool Text::operator==(const char* s) const {
    return *this == Text(s);
  }

  inline bool Text::operator!=(const Text& s) const {
    return !(*this == s);
  }
  inline bool Text::operator!=(const char* s) const {
    return !(*this == s);
  }

  inline bool Text::isValidInt() const {
    if(size() == 0 || (size() == 1 && l[0] == '-')) return false;
    for(std::size_t i = 0; i < size(); i ++) {
      if(l[i] < '0' || l[i] > '9') return false;
    }
    return true;
  }

  inline long long Text::toInt() const {
    if(!isValidInt())
      throw std::invalid_argument("Not an integer");
    
    signed sign = 1;
    std::size_t i = 0;

    if(l[0] == '-') {
      i = 1;
      sign = -1;
    }

    long long r = 0;
    for(; i < size(); i ++) {
      r = r * 10 + (l[i] - '0');
    }

    return r * sign;
  }

  inline Text Text::fromInt(long long i) {
    if(i == 0) return "0";

    Text r;
    bool neg = i < 0;
    if(neg) i = -i;

    while(i > 0) {
      char c = (i % 10) + '0';
      r.insert(0, c);
      i /= 10;
    }

    if(neg) r.insert(0, '-');

    return r;
  }

  inline darray<Text> Text::split(char c) const {
    darray<Text> out;
    Text temp;
    std::size_t stringLength = size();
    for(std::size_t i = 0; i < stringLength; i ++) {
      if(l[i] == c) {
        out.add(temp);
        temp = "";
        continue;
      }

      temp += l[i];
    }
    out.add(temp);
    return out;
  }

  inline Text& Text::insert(std::size_t at, char c) {
    l.insert(at, c);
    return *this;
  }

  inline std::ostream& operator<<(std::ostream& stream, const Text& s) {
    for(std::size_t i = 0; i < s.size(); i ++) {
      stream << s[i];
    }
    return stream;
  }

  inline bool operator==(const char* sraw, const Text& s) {
    return s == sraw;
  }

  inline std::istream& operator>>(std::istream& stream, Text& s) {
    s.clear();

    char c;
    while(stream.get(c)) {
      if(c == ' ' || c == '\n' || c == '\t') break;

      s += c;
    }

    return stream;
  }

  inline const char* Text::raw() const{
    return l.size() ? &l[0] : "";
  }

  inline std::istream& getLine(std::istream& stream, clib::Text& s, char delim = '\n') {
    s.clear();
    char c;
    while(stream.get(c)) {
        if(c == delim) break;
        s += c;
    }
    return stream;
  }


}

