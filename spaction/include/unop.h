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

#ifndef SPACTION_INCLUDE_UNOP_H_
#define SPACTION_INCLUDE_UNOP_H_

#include <string>
#include "cltl.h"

typedef enum {
    NOT,
    NEXT
} unop_type;

class unop : public cltl_formula {
 public:
    unop(unop_type, const cltl_formula *formula);
    ~unop();

    unop(const unop &op) = delete;
    unop &operator= (const unop &op) = delete;

    cltl_formula *clone() const;

    inline void accept(cltl_visitor &visitor) const override { visitor.visit(this); }

    std::string dump() const;

    unop_type get_type() const { return _type; }
    const cltl_formula *sub() const { return _son; }

 private:
    unop_type _type;
    const cltl_formula * _son;
};

#endif  // SPACTION_INCLUDE_UNOP_H_
