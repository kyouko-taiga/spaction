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

class AtomicProposition;
class ConstantExpression;
class UnaryOperator;
class BinaryOperator;

typedef std::shared_ptr<CltlFormula> CltlFormulaPtr;

/// A class that represents a Cost LTL formula.
class CltlFormula : public std::enable_shared_from_this<CltlFormula> {
 public:
    enum FormulaType : short {
        kAtomicProposition,
        kConstantExpression,
        kUnaryOperator,
        kBinaryOperator
    };

    /// Returns the type of the formula so it can be casted to the correct subclass.
    virtual const FormulaType formula_type() const = 0;

    /// Returns whether or not `rhs` is equal to this formula.
    virtual bool operator==(const CltlFormula &rhs) const = 0;

    /// Returns whether or not `rhs` is syntactically equivalent to this formula.
    /// @remarks
    ///     Subclasses may override this method to implement the specific behaviour of their own
    ///     syntactic equivalences. By the default, this method returns true if and only if `rhs`
    ///     is equal to the formula, as defined by `==`.
    virtual inline bool syntactic_eq(const CltlFormula &rhs) const { return (*this == rhs); }

    /// Returns a equivalent formula in negation normal form.
    virtual inline CltlFormulaPtr to_nnf() { return shared_from_this(); }

    /// @remarks
    ///     We could make this method constant, but it would require to pass a
    ///     std::shared_ptr<const CltlFormula> to the visitor and thus to every subsequent call
    ///     using our reference (eg. CltlFormulaFactory).
    virtual void accept(CltlFormulaVisitor &visitor) = 0;

    /// Returns a string representation of the formula.
    virtual std::string dump() const = 0;

 protected:
    /// Virtual destructor.
    /// @remarks
    ///     This destructor will be called by the creator of the object, once it is no more
    ///     referenced by anyone. Subclasses should implement the specific behaviour related to
    ///     their own deallocation within this destructor.
    virtual ~CltlFormula() = 0;

 private:
    friend class CltlFormulaFactory;
};

}  // namespace spaction

#endif  // SPACTION_INCLUDE_CLTLFORMULA_H_
