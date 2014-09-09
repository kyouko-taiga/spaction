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
class binop;
class cltl_formula;
class constant;
class unop;

typedef enum {
    kAtom,
    kConstant,
    kUnaryOperator,
    kBinaryOperator
} FormulaType;

class cltl_visitor {
 public:
    virtual ~cltl_visitor() = 0;

    virtual void visit(const cltl_formula *node);

    virtual void visit(const atomic *node) = 0;
    virtual void visit(const constant *node) = 0;
    virtual void visit(const binop *node) = 0;
    virtual void visit(const unop *node) = 0;
};


/// A class to represent a Cost LTL formula.
class cltl_formula {
 protected:
    virtual ~cltl_formula() = 0;

 public:
    virtual cltl_formula *clone() const = 0;
    virtual void destroy() const = 0;

    /// Returns the type of the formula so it can be casted to the correct subclass.
    virtual const FormulaType get_formula_type() const = 0;

    virtual inline void accept(cltl_visitor &visitor) const { visitor.visit(this); }

    virtual std::string dump() const = 0;
};

/// A factory for cost LTL formulae.
class cltl_factory {
 public:
    static cltl_formula *make_atomic(const std::string &);
    static cltl_formula *make_constant(bool b);
    static cltl_formula *make_next(const cltl_formula *formula);
    static cltl_formula *make_not(const cltl_formula *formula);
    static cltl_formula *make_and(const cltl_formula *left, const cltl_formula *right);
    static cltl_formula *make_or(const cltl_formula *left, const cltl_formula *right);
    static cltl_formula *make_until(const cltl_formula *left, const cltl_formula *right);
    static cltl_formula *make_release(const cltl_formula *left, const cltl_formula *right);
    static cltl_formula *make_costuntil(const cltl_formula *left, const cltl_formula *right);
    static cltl_formula *make_costrelease(const cltl_formula *left, const cltl_formula *right);

    static cltl_formula *make_imply(const cltl_formula *left, const cltl_formula *right);
    static cltl_formula *make_globally(const cltl_formula *formula);
    static cltl_formula *make_finally(const cltl_formula *formula);
    static cltl_formula *make_costfinally(const cltl_formula *formula);
};

}  // namespace spaction

#endif  // SPACTION_INCLUDE_CLTL_H_
