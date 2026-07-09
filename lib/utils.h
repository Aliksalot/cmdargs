#pragma once

#define TODO(text) throw (text);

namespace clib {

  template<typename T>
  T&& move(T& x) {
    return static_cast<T&&>(x);
  }
}
