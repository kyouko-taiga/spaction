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

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace spaction {

class atomic;
class binop;
class cltl_formula;
class cltl_factory;
class constant;
class unop;

typedef enum {
    kAtom,
    kConstant,
    kUnaryOperator,
    kBinaryOperator
} FormulaType;

typedef std::shared_ptr<cltl_formula> cltl_formula_ptr;

class cltl_visitor {
 public:
    virtual ~cltl_visitor() = 0;

    virtual void visit(const std::shared_ptr<const atomic> &node) = 0;
    virtual void visit(const std::shared_ptr<const constant> &node) = 0;
    virtual void visit(const std::shared_ptr<const binop> &node) = 0;
    virtual void visit(const std::shared_ptr<const unop> &node) = 0;
};

/// A class that represents a Cost LTL formula.
class cltl_formula : public std::enable_shared_from_this<cltl_formula> {
 public:
    /// Returns the type of the formula so it can be casted to the correct subclass.
    virtual const FormulaType get_formula_type() const = 0;

    virtual bool operator==(const cltl_formula &rhs) const;
    virtual bool syntactic_eq(const cltl_formula &rhs) const;

    /// Returns a equivalent formula in negation normal form.
    virtual inline cltl_formula_ptr to_nnf() { return shared_from_this(); }

    virtual void accept(cltl_visitor &visitor) const = 0;

    virtual std::string dump() const = 0;

 protected:
    /// Virtual destructor.
    /// @remarks
    ///     This destructor will be called by the creator of the object, once it is no more
    ///     referenced by anyone. Subclasses should implement the specific behaviour related to
    ///     their own deallocation within this destructor.
    virtual ~cltl_formula() = 0;

 private:
    friend class cltl_factory;
};

/// A factory class for Cost LTL formulae.
class cltl_factory {
 public:
    cltl_formula_ptr make_atomic(const std::string &);
    cltl_formula_ptr make_constant(bool b);

    cltl_formula_ptr make_next(const cltl_formula_ptr &formula);
    cltl_formula_ptr make_not(const cltl_formula_ptr &formula);
    cltl_formula_ptr make_and(const cltl_formula_ptr &left, const cltl_formula_ptr &right);
    cltl_formula_ptr make_or(const cltl_formula_ptr &left, const cltl_formula_ptr &right);
    cltl_formula_ptr make_until(const cltl_formula_ptr& left, const cltl_formula_ptr &right);
    cltl_formula_ptr make_release(const cltl_formula_ptr &left, const cltl_formula_ptr &right);
    cltl_formula_ptr make_costuntil(const cltl_formula_ptr &left, const cltl_formula_ptr &right);
    cltl_formula_ptr make_costrelease(const cltl_formula_ptr &left, const cltl_formula_ptr &right);

    cltl_formula_ptr make_imply(const cltl_formula_ptr &left, const cltl_formula_ptr &right);
    cltl_formula_ptr make_globally(const cltl_formula_ptr &formula);
    cltl_formula_ptr make_finally(const cltl_formula_ptr &formula);
    cltl_formula_ptr make_costfinally(const cltl_formula_ptr &formula);

 private:
    /// Stores the unique index.
    std::unordered_set<cltl_formula*> _formulae;

    cltl_formula_ptr _make_shared_formula(cltl_formula* formula);

    /// Removes a formula from the unique index once it is no more referenced.
    /// @remarks
    ///     This custom deleter is bound to the shared pointers built by this factory. It gets
    ///     called when the references counter of a particular shared pointer reaches 0.
    void _deleter(cltl_formula *formula) {
        _formulae.erase(formula);
        delete formula;
    }
};

}  // namespace spaction

#endif  // SPACTION_INCLUDE_CLTL_H_
