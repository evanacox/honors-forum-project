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