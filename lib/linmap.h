#pragma once

#include"darray.h"
#include"primitive.h"

namespace clib {
  template<typename K, typename V>
  class linmap {
  public:
    linmap& set(const K& key, const V& value);
    V& get(const K& key) const;
    V& operator[](const K& key) const;
    K& getKey(const V& value) const;
    bool hasKey(const K& key) const;
    bool hasValue(const K& value) const;

    std::size_t count() const;
  private:
    darray<tuple<K,V>> data;
  };

  template<typename K, typename V>
  inline linmap<K,V>& linmap<K,V>::set(const K& key, const V& value) {
    for(auto& kv: data) {
      if(kv.left == key) {
        kv.right = value;
        return *this;
      }
    }
    data.add(tuple{key, value});
    return *this;
  }

  template<typename K, typename V>
  inline V& linmap<K,V>::get(const K& key) const {
    for(auto& kv: data) {
      if(kv.left == key) {
        return kv.right;
      }
    }
    throw std::invalid_argument("Couldn't match key to value");
  }

  template<typename K, typename V>
  inline V& linmap<K,V>::operator[](const K& key) const {
    return get(key);
  }

  template<typename K, typename V>
  inline K& linmap<K,V>::getKey(const V& value) const {
    for(auto& kv: data) {
      if(kv.right == value) {
        return kv.left;
      }
    }
    throw std::invalid_argument("Couldn't match value to key");
  }

  template<typename K, typename V>
  inline bool linmap<K,V>::hasKey(const K& key) const {
    try{
      get(key);
      return true;
    } catch(...) {
      return false;
    }
  }

  
  template<typename K, typename V>
  inline std::size_t linmap<K,V>::count() const {
    return data.size();
  }

  template<typename K, typename V>
  inline bool linmap<K,V>::hasValue(const K& value) const {
    try{
      getKey(value);
      return true;
    } catch(...) {
      return false;
    }
  }
}
