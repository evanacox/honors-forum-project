//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./ticket_mutex.h"

namespace gal {
  void TicketMutex::lock() noexcept {
    const auto ticket_number = count_.fetch_add(1, std::memory_order_relaxed);

    // if it's our turn, we can skip the waiting and just lock straight away
    if (current_.load(std::memory_order_relaxed) == ticket_number) {
      lock_.lock();
      return;
    }

    // wait until we can "lock" it, then plug it into a condvar and wait.
    std::unique_lock<std::mutex> lock(lock_);

    // once this returns, we own the lock
    waiter_.wait(lock, [&] {
      return ticket_number == current_.load(std::memory_order_relaxed);
    });

    // lock is owned, now we release it and evaporate the pointer
    // so that it remains locked when we return
    (void)lock.release();
  }

  void TicketMutex::unlock() noexcept {
    // update the current ticket number
    current_.fetch_add(1, std::memory_order_relaxed);

    lock_.unlock();

    // notify all currently waiting to check the new ticket number
    waiter_.notify_all();
  }
} // namespace gal