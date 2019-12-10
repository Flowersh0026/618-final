#ifndef _LOCK_FREE_ALLOCATOR_H_
#define _LOCK_FREE_ALLOCATOR_H_

#include <new>
#include "free_list.h"

template <typename T>
class LockFreeAllocator {
 public:
  using value_type = T;

  LockFreeAllocator() noexcept : free_list_() {}

  template <class U>
  LockFreeAllocator(const LockFreeAllocator<U>&) noexcept
      : LockFreeAllocator() {}

  ~LockFreeAllocator() {
    while (true) {
      value_type* p = free_list_.Pop();
      if (p) {
        ::operator delete(p);
      } else {
        break;
      }
    }
  }

  value_type* allocate(std::size_t n) {
    if (n != 1) {
      throw std::bad_alloc();  // only support allocate objects one-by-one
    }
    value_type* p = free_list_.Pop();
    if (p == nullptr) {
      // the return pointer of `operator new` is properly aligned
      p = static_cast<value_type*>(::operator new(sizeof(value_type)));
    }
    return p;
  }

  void deallocate(value_type* p, std::size_t n) noexcept {
    assert(n == 1);
    free_list_.Push(p);
  }

 private:
  FreeList<value_type> free_list_;
};

template <class T, class U>
bool operator==(const LockFreeAllocator<T>& a,
                const LockFreeAllocator<U>& b) noexcept {
  return std::is_same<T, U>::value && (&a) == (&b);
}

template <class T, class U>
bool operator!=(const LockFreeAllocator<T>& a,
                const LockFreeAllocator<U>& b) noexcept {
  return !(a == b);
}

#endif  // _LOCK_FREE_ALLOCATOR_H_
