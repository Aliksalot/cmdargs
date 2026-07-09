#pragma once

#include<cstdlib>
#include<stdexcept>

#include"utils.h"

namespace clib {

  template<typename T>
  class darray {

    public:
      /*Since move_if_noexcept is forbidden, convetion
       *is that moving objects is well-defined and is noexcept for all types.*/
      darray();
      darray(const T* arr, std::size_t len);
      ~darray();

      darray& add(const T& item);
      darray& add(T&& item);

      darray(const darray& l);
      darray& operator=(const darray& l);
      darray(darray&& l) noexcept;
      darray& operator=(darray&& l) noexcept;

      T& operator[](std::size_t index);
      const T& operator[](std::size_t index) const;

      void extend(const darray<T>& list);
      void clear();
      darray& remove(std::size_t st, std::size_t items = 1);
      darray& insert(std::size_t at, const T& item);
      T pop();
      T pop(std::size_t index);
      bool contains(const T& item) const;
      std::size_t size() const;
      long indexOf(const T& item) const;
      bool empty() const;
      
      T* begin();
      T* end();

      const T* begin() const;
      const T* end() const;

    private:
      T* data = nullptr;
      std::size_t count = 0;
      std::size_t cap = 0;

      void expand();
      void reserve(std::size_t space);
      bool indexInBounds(std::size_t index) const;
      bool full() const;

      /**Destroys all objects in data, starting from std::size_t st
       * and sets count to 0. DOESN'T free memory, allocated in data[] */
      void clear_data(std::size_t st = 0);
      /**Calls delete on data. Does NOT destruct T objects, only frees memory*/
      void delete_data();
      /**
       * Allocates memory and stores address in data pointer.
       * Doesn't call T constructor.
       * */
      T* construct_data(std::size_t size);
  };

  //-------DECLARATIONS------

  template<typename T>
  void darray<T>::clear() {
    clear_data();
  }

  template<typename T>
  void darray<T>::delete_data() {
    ::operator delete(data);
    data = nullptr;
  }

  template<typename T>
  T* darray<T>::construct_data(std::size_t size) {
    return static_cast<T*>(::operator new(sizeof(T) * size));
  }
  template<typename T>
  void darray<T>::clear_data(std::size_t st) {
    if(st >= count) return;

    for(std::size_t i = st; i < count; i ++) {
      data[i].~T();
    }
    count = st;
  }

  template<typename T>
  void darray<T>::reserve(std::size_t space) {

    if(space <= cap) return;

    T* temp = construct_data(space);
    std::size_t tempCount = count;

    for(std::size_t i = 0; i < count; i ++) {
      new (temp + i) T(move(data[i]));
    }

    clear_data();
    delete_data();

    data = temp;
    count = tempCount;
    cap = space;
  }

  template<typename T>
  void darray<T>::expand() {
    reserve(cap == 0 ? 1 : cap * 2);
  }

  template<typename T>
  darray<T>::darray(const T* arr, std::size_t len) {
    reserve(len);
    
    for(std::size_t i = 0; i < len; i ++) {
      new (data + i) T(arr[i]);
    }

    count = len;
  }

  template<typename T>
  darray<T>::darray() {}

  template<typename T>
  darray<T>::~darray() {
    clear_data();
    delete_data();
  }

  template<typename T>
  darray<T>& darray<T>::add(const T& item) {
    if(full()) expand();

    new (data + count) T(item);
    count ++; 

    return *this;
  }

  template<typename T>
  darray<T>& darray<T>::add(T&& item) {
    if(full()) expand();

    new (data + count) T(move(item));

    count ++; 

    return *this;
  }

  template<typename T>
  void darray<T>::extend(const darray<T>& list) {

    reserve(count + list.size());
    std::size_t initialCount = count;
    try{
      for(std::size_t i = 0; i < list.size(); i ++){
        new (data + count) T(list[i]);
        count ++;
      }
    }catch(...) {
      clear_data(initialCount);
      throw;
    }
   };

  template<typename T>
  darray<T>::darray(const darray& l) {
    reserve(l.cap);
    count = l.count;
    for(std::size_t i = 0; i < l.count; i ++) {
      new (data + i) T(l.data[i]);
    }
  }
  template<typename T>
  darray<T>& darray<T>::operator=(const darray& l) {
    if(&l == this) return *this;

    clear_data();
    reserve(l.cap);

    for(std::size_t i = 0; i < l.count; i ++) {
      new (data + i) T(l.data[i]);
      count ++;
    }
    return *this;
  }

  template<typename T>
  darray<T>::darray(darray&& l) noexcept{
    data = l.data;
    count = l.count;
    cap = l.cap;
    l.data = nullptr;
    l.cap = 0;
    l.count = 0;
  }

  template<typename T>
  darray<T>& darray<T>::remove(std::size_t st, std::size_t items) {
    if(st >= count || items == 0 || items > count - st) return *this;
    
    for(std::size_t i = st; i < count - items; i ++) {
      data[i] = move(data[items + i]);
    }

    clear_data(count - items);
    return *this;
  }

  template<typename T>
  darray<T>& darray<T>::insert(std::size_t at, const T& item) { 
    if(at == count) return add(item);

    if(!indexInBounds(at)) throw std::out_of_range("insert: index out of bounds");

    if(full()) expand();
    
    new (data + count) T(move(data[count - 1]));
    for(std::size_t i = count - 1; i > at; i --) {
      data[i] = move(data[i-1]);
    }
    data[at] = item;
    count ++;

    return *this;
  }

  template<typename T>
  T darray<T>::pop() {
    if(count == 0)
      throw std::out_of_range("list pop: can't pop on an empty list");

    T temp = move(data[count - 1]);
    clear_data(count - 1);

    return temp;
  }

  template<typename T>
  T darray<T>::pop(std::size_t index) {
    if(index >= count)
      throw std::out_of_range("list pop: index out of bounds");

    T obj = move(data[index]);

    for(std::size_t i = index; i < count - 1; ++i) {
      data[i] = std::move(data[i + 1]);
    }

    clear_data(count - 1);
    return obj;
  }

  template<typename T>
  long darray<T>::indexOf(const T& item) const {
    for(std::size_t i = 0; i < count; i ++) {
      if(data[i] == item) {
        return i;
      }
    }
    return -1;
  }

  template<typename T>
  darray<T>& darray<T>::operator=(darray&& l) noexcept{
    if(this == &l) return *this;

    clear_data();
    delete_data();

    data = l.data;
    count = l.count;
    cap = l.cap;

    l.data = nullptr;
    l.cap = 0;
    l.count = 0;

    return *this;
  }

  template<typename T>
  T& darray<T>::operator[](std::size_t index) {
    if(indexInBounds(index)) {
        return data[index];
    }
    throw std::out_of_range("index out of bounds");
  }

  template<typename T>
  const T& darray<T>::operator[](std::size_t index) const{
    if(indexInBounds(index)) {
        return data[index];
    }
    throw std::out_of_range("index out of bounds");
  }


  template<typename T>
  bool darray<T>::contains(const T& item) const {
    for(std::size_t i = 0; i < count; i ++) {
      if(data[i] == item) return true;
    }
    return false;
  }

  template<typename T>
  std::size_t darray<T>::size() const{
    return count;
  }

  template<typename T>
  bool darray<T>::indexInBounds(std::size_t index) const {
    return index < count;
  }

  template<typename T>
  bool darray<T>::full() const {
    return cap == count;
  }

  template<typename T>
  bool darray<T>::empty() const {
    return count == 0;
  }
  template<typename T>
  T* darray<T>::begin() {
    return data;
  }
  template<typename T>
  T* darray<T>::end() {
    return data + count;
  }
  template<typename T>
  const T* darray<T>::begin() const {
    return data;
  }
  template<typename T>
  const T* darray<T>::end() const {
    return data + count;
  }

}

