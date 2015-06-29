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

#ifndef SPACTION_INCLUDE_BINARYOPERATOR_H_
#define SPACTION_INCLUDE_BINARYOPERATOR_H_

#include <unordered_set>
#include "CltlFormula.h"

namespace spaction {

class CltlFormulaFactory;
class CltlFormulaVisitor;

class BinaryOperator : public CltlFormula {
 public:
    enum BinaryOperatorType : char {
        kUntil,
        kRelease,
        kCostUntil,
        kCostRelease
    };

    /// Copy construction is forbidden.
    BinaryOperator(const BinaryOperator &) = delete;
    /// Copy assignement is forbidden.
    BinaryOperator &operator= (const BinaryOperator &) = delete;

    inline const FormulaType formula_type() const override { return kBinaryOperator; };
    BinaryOperatorType operator_type() const { return _type; }

    /// Returns whether or not `rhs` is syntactically equivalent to this formula.
    virtual bool syntactic_eq(const CltlFormula &rhs) const;

    /// Returns a equivalent formula in negation normal form.
    virtual CltlFormulaPtr to_nnf();

    /// Returns a equivalent formula in disjunctive normal form.
    virtual CltlFormulaPtr to_dnf();

    inline const CltlFormulaPtr &left() const { return _left; }
    inline const CltlFormulaPtr &right() const { return _right; }

    void accept(CltlFormulaVisitor &visitor) override;

    inline std::size_t height() const { return 1 + std::max(_left->height(), _right->height()); }

    bool is_infltl() const;
    bool is_supltl() const;
    bool is_propositional() const;
    bool is_nnf() const;

    std::string dump() const override;

 protected:
    explicit BinaryOperator(BinaryOperatorType type, const CltlFormulaPtr &left,
                            const CltlFormulaPtr &right, CltlFormulaFactory *creator);
    ~BinaryOperator() { }

 private:
    friend class CltlFormulaFactory;

    BinaryOperatorType _type;
    const CltlFormulaPtr _left;
    const CltlFormulaPtr _right;

    /// Internal method to retrieve the leaves of this formula.
    /// @remarks
    ///     A subformula of a binary operation is called a leaf if it is either a binary operation
    ///     whose operator differs, or any other kinf of formula. For instance, the expression
    ///     `(a + (b * c)) + d` has for leaves `a` `ab` and `d`.
    ///
    /// @warning
    ///     The result of this method is valid only if the `_type` of this formula is an operator
    ///     of propositional logic.
    std::unordered_multiset<const CltlFormula*> _leaves() const;
};

}  // namespace spaction

#endif  // SPACTION_INCLUDE_BINARYOPERATOR_H_
