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

#ifndef SPACTION_INCLUDE_BINOP_H_
#define SPACTION_INCLUDE_BINOP_H_

#include <string>
#include "cltl.h"

namespace spaction {

typedef enum {
    AND,
    OR,
    UNTIL,
    RELEASE,
    COST_UNTIL,
    COST_RELEASE
} binop_type;

class binop : public cltl_formula {
    friend class cltl_factory;
 public:
    binop(const binop &op) = delete;
    binop &operator= (const binop &op) = delete;

    inline const FormulaType get_formula_type() const override { return kBinaryOperator; };

    binop_type get_type() const { return _type; }

    inline const cltl_formula_ptr &left() const { return _left; }

    inline const cltl_formula_ptr &right() const { return _right; }

    inline void accept(cltl_visitor &visitor) const override {
        visitor.visit(std::dynamic_pointer_cast<const atomic>(shared_from_this()));
    }

    std::string dump() const override;

 protected:
    explicit binop(binop_type type, const cltl_formula_ptr &left, const cltl_formula_ptr &right);
    ~binop();

 private:
    binop_type _type;
    const cltl_formula_ptr _left;
    const cltl_formula_ptr _right;
};

}  // namespace spaction

#endif  // SPACTION_INCLUDE_BINOP_H_
