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
  gal::TicketMutex console_lock;
}

namespace gal {
  void internal::lock_console() noexcept {
    console_lock.lock();
  }

  void internal::unlock_console() noexcept {
    console_lock.unlock();
  }

  internal::BufferedFakeOstream outs() noexcept {
    auto stream = internal::BufferedFakeOstream(&std::cout);

    stream << colors::bold_cyan("info: ");

    // copy elision kicks in, stream.~BufferedFakeOstream() is not called
    return stream;
  }

  internal::UnbufferedFakeOstream errs() noexcept {
    auto stream = internal::UnbufferedFakeOstream(&std::cerr);

    stream << colors::bold_red("error: ");

    // copy elision kicks in, stream.~BufferedFakeOstream() is not called
    return stream;
  }

  std::ostream& raw_outs() noexcept {
    return std::cout;
  }
} // namespace gal