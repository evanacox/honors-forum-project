//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./log.h"
#include "./ticket_mutex.h"
#include <iostream>

namespace {
  galc::TicketMutex console_lock;
}

namespace galc {
  void internal::lock_console() noexcept {
    console_lock.lock();
  }

  void internal::unlock_console() noexcept {
    console_lock.unlock();
  }

  internal::BufferedFakeOstream outs() noexcept {
    return internal::BufferedFakeOstream(&std::cout);
  }

  internal::UnbufferedFakeOstream errs() noexcept {
    return internal::BufferedFakeOstream(&std::cerr);
  }

  std::ostream& raw_outs() noexcept {
    return std::cout;
  }
} // namespace galc