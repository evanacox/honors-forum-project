//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

namespace gallium {
  ///
  ///
  ///
  class Driver {
  public:
    ///
    ///
    ///
    explicit Driver(int argc, char** argv);

    ///
    ///
    ///
    int start() noexcept;

  private:
    int argc_;
    char** argv_;
  };
} // namespace gallium