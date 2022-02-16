//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021-2022 Evan Cox <evanacox00@gmail.com>. All rights reserved. //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <mutex>

namespace gal {
  /// As close to a "fair" mutex implementation as can be done in user-level code.
  ///
  /// Threads are guaranteed to gain the lock in the exact order they try to lock
  /// it in. When a thread calls `lock`, they are made to wait until the mutex
  /// has been released by the thread that tried to lock it last.
  class TicketMutex {
  public:
    /// Creates a TicketMutex
    explicit TicketMutex() = default;

    /// TicketMutex is not copyable
    TicketMutex(const TicketMutex&) = delete;

    /// TicketMutex is not movable
    TicketMutex(TicketMutex&&) = delete;

    /// TicketMutex is not copy-assignable
    TicketMutex& operator=(const TicketMutex&) = delete;

    /// TicketMutex is not move-assignable
    TicketMutex& operator=(TicketMutex&&) = delete;

    /// Destroys the mutex. Will call `std::terminate`
    /// if the mutex is still locked, or if any threads
    /// are still waiting.
    ~TicketMutex();

    /// Locks the mutex, guaranteed to be fair.
    void lock() noexcept;

    /// Unlocks the mutex, alerts other threads to check if its their turn
    void unlock() noexcept;

  private:
    /// The current ticket being served
    std::atomic<std::size_t> current_ = 0;

    /// The number of tickets given out, also equal to the next ticket number to give
    std::atomic<std::size_t> count_ = 0;

    /// Whether or not the mutex is "locked"
    std::atomic<bool> locked_ = false;

    /// The mutex actually used to wait
    std::mutex lock_;

    /// The object used to notify threads that `current` changed
    std::condition_variable waiter_;
  };
} // namespace gal