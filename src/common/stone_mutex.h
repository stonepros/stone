// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#pragma once

#include <utility>
#include "common/containers.h"

// What and why
// ============
//
// For general code making use of mutexes, use these stone:: types.
// The key requirement is that you make use of the stone::make_mutex()
// and make_recursive_mutex() factory methods, which take a string
// naming the mutex for the purposes of the lockdep debug variant.

#if defined(WITH_SEASTAR) && !defined(WITH_ALIEN)

namespace stone {
  // an empty class satisfying the mutex concept
  struct dummy_mutex {
    void lock() {}
    bool try_lock() {
      return true;
    }
    void unlock() {}
    void lock_shared() {}
    void unlock_shared() {}
  };

  struct dummy_shared_mutex : dummy_mutex {
    void lock_shared() {}
    void unlock_shared() {}
  };

  using mutex = dummy_mutex;
  using recursive_mutex = dummy_mutex;
  using shared_mutex = dummy_shared_mutex;
  // in seastar, we should use a difference interface for enforcing the
  // semantics of condition_variable

  template <typename ...Args>
  dummy_mutex make_mutex(Args&& ...args) {
    return {};
  }

  template <typename ...Args>
  recursive_mutex make_recursive_mutex(Args&& ...args) {
    return {};
  }

  template <typename ...Args>
  shared_mutex make_shared_mutex(Args&& ...args) {
    return {};
  }

  #define stone_mutex_is_locked(m) true
  #define stone_mutex_is_locked_by_me(m) true
}

#else  // defined (WITH_SEASTAR) && !defined(WITH_ALIEN)
//
// For legacy Mutex users that passed recursive=true, use
// stone::make_recursive_mutex.  For legacy Mutex users that passed
// lockdep=false, use std::mutex directly.

#ifdef STONE_DEBUG_MUTEX

// ============================================================================
// debug (lockdep-capable, various sanity checks and asserts)
// ============================================================================

#include "common/condition_variable_debug.h"
#include "common/mutex_debug.h"
#include "common/shared_mutex_debug.h"

namespace stone {
  typedef stone::mutex_debug mutex;
  typedef stone::mutex_recursive_debug recursive_mutex;
  typedef stone::condition_variable_debug condition_variable;
  typedef stone::shared_mutex_debug shared_mutex;

  // pass arguments to mutex_debug ctor
  template <typename ...Args>
  mutex make_mutex(Args&& ...args) {
    return {std::forward<Args>(args)...};
  }

  // pass arguments to recursive_mutex_debug ctor
  template <typename ...Args>
  recursive_mutex make_recursive_mutex(Args&& ...args) {
    return {std::forward<Args>(args)...};
  }

  // pass arguments to shared_mutex_debug ctor
  template <typename ...Args>
  shared_mutex make_shared_mutex(Args&& ...args) {
    return {std::forward<Args>(args)...};
  }

  // debug methods
  #define stone_mutex_is_locked(m) ((m).is_locked())
  #define stone_mutex_is_not_locked(m) (!(m).is_locked())
  #define stone_mutex_is_rlocked(m) ((m).is_rlocked())
  #define stone_mutex_is_wlocked(m) ((m).is_wlocked())
  #define stone_mutex_is_locked_by_me(m) ((m).is_locked_by_me())
  #define stone_mutex_is_not_locked_by_me(m) (!(m).is_locked_by_me())
}

#else

// ============================================================================
// release (fast and minimal)
// ============================================================================

#include <condition_variable>
#include <mutex>
#include <shared_mutex>


namespace stone {

  typedef std::mutex mutex;
  typedef std::recursive_mutex recursive_mutex;
  typedef std::condition_variable condition_variable;
  typedef std::shared_mutex shared_mutex;

  // discard arguments to make_mutex (they are for debugging only)
  template <typename ...Args>
  std::mutex make_mutex(Args&& ...args) {
    return {};
  }
  template <typename ...Args>
  std::recursive_mutex make_recursive_mutex(Args&& ...args) {
    return {};
  }
  template <typename ...Args>
  std::shared_mutex make_shared_mutex(Args&& ...args) {
    return {};
  }

  // debug methods.  Note that these can blindly return true
  // because any code that does anything other than assert these
  // are true is broken.
  #define stone_mutex_is_locked(m) true
  #define stone_mutex_is_not_locked(m) true
  #define stone_mutex_is_rlocked(m) true
  #define stone_mutex_is_wlocked(m) true
  #define stone_mutex_is_locked_by_me(m) true
  #define stone_mutex_is_not_locked_by_me(m) true

}

#endif	// STONE_DEBUG_MUTEX

#endif	// WITH_SEASTAR

namespace stone {

template <class LockT,
          class LockFactoryT>
stone::containers::tiny_vector<LockT> make_lock_container(
  const std::size_t num_instances,
  LockFactoryT&& lock_factory)
{
  return {
    num_instances, [&](const std::size_t i, auto emplacer) {
      // this will be called `num_instances` times
      new (emplacer.data()) LockT {lock_factory(i)};
    }
  };
}
} // namespace stone

