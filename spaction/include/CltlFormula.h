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

#ifndef SPACTION_INCLUDE_CLTLFORMULA_H_
#define SPACTION_INCLUDE_CLTLFORMULA_H_

#include <memory>
#include <string>

namespace spaction {

class CltlFormula;
class CltlFormulaFactory;
class CltlFormulaVisitor;

typedef std::shared_ptr<CltlFormula> CltlFormulaPtr;

/// A class that represents a Cost LTL formula.
class CltlFormula : public std::enable_shared_from_this<CltlFormula> {
 public:
    enum FormulaType : char {
        kAtomicProposition,
        kConstantExpression,
        kUnaryOperator,
        kBinaryOperator
    };

    /// Returns a pointer to the factory that created this formula.
    virtual inline CltlFormulaFactory *creator() const { return _creator; }

    /// Returns the type of the formula so it can be casted to the correct subclass.
    virtual const FormulaType formula_type() const = 0;

    /// Returns whether or not `rhs` is the same formula.
    virtual inline bool operator==(const CltlFormula &rhs) const { return this == &rhs; }
    /// Returns whether or not `rhs` is not the same formula.
    virtual inline bool operator!=(const CltlFormula &rhs) const { return !(*this == rhs); }

    /// Returns whether or not `rhs` is syntactically equivalent to this formula.
    virtual inline bool syntactic_eq(const CltlFormula &rhs) const = 0;

    /// Returns a equivalent formula in negation normal form.
    virtual inline CltlFormulaPtr to_nnf() { return shared_from_this(); }

    /// Returns a equivalent formula in disjunctive normal form.
    virtual inline CltlFormulaPtr to_dnf() { return this->to_nnf(); }

    /// @note
    ///     We could make this method constant, but it would require to pass a
    ///     std::shared_ptr<const CltlFormula> to the visitor and thus to every subsequent call
    ///     using our reference (eg. CltlFormulaFactory).
    virtual void accept(CltlFormulaVisitor &visitor) = 0;

    /// Returns the height of the formula.
    ///
    /// Height of a formula is `1` for an atomic proposition, or the highest height within its
    /// subformulae + `1`.
    virtual std::size_t height() const = 0;

    /// Returns a string representation of the formula.
    virtual std::string dump() const = 0;

 protected:
    CltlFormulaFactory *_creator;

    /// Class constructor.
    explicit CltlFormula(CltlFormulaFactory *creator) : _creator(creator) { }

    /// Virtual destructor.
    ///
    /// This destructor will be called by the creator of the object, once it is no more referenced
    /// by anyone. Subclasses should implement the specific behaviour related to their own
    /// deallocation within this destructor.
    virtual ~CltlFormula() { }

    /// Comparison operator used internally to build canonical forms.
    virtual inline bool operator<(const CltlFormula &rhs) const { return this < &rhs; }
    /// Comparison operator used internally to build canonical forms.
    virtual inline bool operator>(const CltlFormula &rhs) const { return this > &rhs; }

 private:
    friend class CltlFormulaFactory;
};

}  // namespace spaction

#endif  // SPACTION_INCLUDE_CLTLFORMULA_H_
