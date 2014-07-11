// This source file is part of spaction
//
// Copyright 2014 Software Modeling and Verification Group
// University of Geneva
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SPACTION_INCLUDE_CLTL_H_
#define SPACTION_INCLUDE_CLTL_H_

#include <string>

namespace spaction {

class atomic;
class constant;
class binop;
class costop;
class unop;

class cltl_visitor {
 public:
    virtual ~cltl_visitor() = 0;

    virtual void visit(const atomic *node) = 0;
    virtual void visit(const constant *node) = 0;
    virtual void visit(const binop *node) = 0;
    virtual void visit(const unop *node) = 0;
};

/**
 *  A class to represent a Cost LTL formula
 */
class cltl_formula {
 public:
    virtual ~cltl_formula() = 0;

    virtual void accept(cltl_visitor& visitor) const = 0;

    virtual std::string dump() const = 0;

    virtual cltl_formula *clone() const = 0;
    // virtual cltl_formula *destroy() const = 0;
};

}  // namespace spaction

#endif  // SPACTION_INCLUDE_CLTL_H_
