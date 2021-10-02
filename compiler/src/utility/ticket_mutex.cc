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

namespace galc {
  void TicketMutex::lock() noexcept {
    const auto ticket_number = state_.count.fetch_add(1, std::memory_order_relaxed);

    // if it's our turn, we can skip the waiting and just lock straight away
    if (state_.current.load(std::memory_order_relaxed) == ticket_number) {
      state_.lock.lock();
      return;
    }

    // wait until we can "lock" it, then plug it into a condvar and wait.
    std::unique_lock<std::mutex> lock(state_.lock);

    // once this returns, we own the lock
    state_.waiter.wait(lock, [&] {
      return ticket_number == state_.current.load(std::memory_order_relaxed);
    });

    // lock is owned, now we release it and evaporate the pointer
    // so that it remains locked when we return
    (void)lock.release();
  }

  void TicketMutex::unlock() noexcept {
    // update the current ticket number
    state_.current.fetch_add(1, std::memory_order_relaxed);

    state_.lock.unlock();

    // notify all currently waiting to check the new ticket number
    state_.waiter.notify_all();
  }

  TicketMutex::native_handle_type TicketMutex::native_handle() noexcept {
    return &state_;
  }
} // namespace galc