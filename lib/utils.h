#pragma once

#include<iostream>

namespace clib {

  template<typename T>
  T&& move(T& x) {
    return static_cast<T&&>(x);
  }
}
