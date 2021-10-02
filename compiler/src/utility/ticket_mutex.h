//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>                            //
//                                                                           //
// Licensed under the Apache License, Version 2.0 (the "License");           //
// you may not use this file except in compliance with the License.          //
// You may obtain a copy of the License at                                   //
//                                                                           //
//    http://www.apache.org/licenses/LICENSE-2.0                             //
//                                                                           //
// Unless required by applicable law or agreed to in writing, software       //
// distributed under the License is distributed on an "AS IS" BASIS,         //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  //
// See the License for the specific language governing permissions and       //
// limitations under the License.                                            //
//                                                                           //
//======---------------------------------------------------------------======//

#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <mutex>

namespace galc {
  class TicketMutex {
    struct State {
      /// The current ticket being served
      std::atomic<std::size_t> current = 0;

      /// The number of tickets given out, also equal to the next ticket number to give
      std::atomic<std::size_t> count = 0;

      /// The mutex actually used to wait
      std::mutex lock;

      /// The object used to notify threads that `current` changed
      std::condition_variable waiter;
    };

  public:
    using native_handle_type = State*;

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

    /// Destroys the mutex
    ~TicketMutex() = default;

    /// Locks the mutex, guaranteed to be fair.
    void lock() noexcept;

    /// Unlocks the mutex, alerts other threads to check if its their turn
    void unlock() noexcept;

    /// Returns the mutex's state
    native_handle_type native_handle() noexcept;

  private:
    State state_;
  };
} // namespace galc