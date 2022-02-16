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

#include "llvm/IR/Value.h"

namespace gal::backend {
  enum class StorageLoc { mem, reg };

  /// Contains information about the storage location of a value, used for
  /// code that deals with moving in/out of registers and getting addresses of objects
  class StoredValue {
  public:
    // I am intentionally *trying* to get implicit conversions here, to minimize impact on
    // the actual code generator. Most places don't need to care about this, only
    // the specific places generating allocas need to specify anything extra
    StoredValue(llvm::Value* value, StorageLoc loc = StorageLoc::reg) noexcept // NOLINT(google-explicit-constructor)
        : value_{value},
          loc_{loc} {}

    // see above: I want the implicit conversion
    operator llvm::Value*() const noexcept { // NOLINT(google-explicit-constructor)
      return value_;
    }

    [[nodiscard]] StorageLoc loc() const noexcept {
      return loc_;
    }

    [[nodiscard]] llvm::Type* type() const noexcept {
      return value_->getType();
    }

    [[nodiscard]] llvm::Value* value() const noexcept {
      return value_;
    }

  private:
    llvm::Value* value_;
    StorageLoc loc_;
  };
} // namespace gal::backend