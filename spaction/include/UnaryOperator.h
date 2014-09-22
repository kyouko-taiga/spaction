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

#ifndef SPACTION_INCLUDE_UNARYOPERATOR_H_
#define SPACTION_INCLUDE_UNARYOPERATOR_H_

#include "CltlFormula.h"

namespace spaction {

class CltlFormulaFactory;
class CltlFormulaVisitor;

class UnaryOperator : public CltlFormula {
 public:
    enum UnaryOperatorType : char {
        kNot,
        kNext
    };

    /// Copy construction is forbidden.
    UnaryOperator(const UnaryOperator &) = delete;
    /// Copy assignement is forbidden.
    UnaryOperator &operator= (const UnaryOperator &) = delete;

    inline const FormulaType formula_type() const override { return kUnaryOperator; };
    UnaryOperatorType operator_type() const { return _type; }

    /// Returns true if `rhs` is an UnaryOperator whose operator and operand are equal to ours.
    virtual bool operator==(const CltlFormula &rhs) const;

    /// Returns a equivalent formula in negation normal form.
    virtual CltlFormulaPtr to_nnf();

    inline const CltlFormulaPtr &operand() const { return _operand; }

    void accept(CltlFormulaVisitor &visitor) override;

    std::string dump() const override;

 protected:
    explicit UnaryOperator(UnaryOperatorType type, const CltlFormulaPtr &operand,
                           CltlFormulaFactory *creator);
    ~UnaryOperator() { }

    /// Comparison operator used internally to build normal forms.
    ///
    /// Formulae ordering is first based on formula type (@see CltlFormula::FormulaType). Then, the
    /// ordering of Unary operator is based on operator type (@see UnaryOperatorType) and finally on
    /// the relation of operands.
    virtual bool operator<(const CltlFormula &rhs) const;

 private:
    friend class CltlFormulaFactory;

    UnaryOperatorType _type;
    const CltlFormulaPtr _operand;
};

}  // namespace spaction

#endif  // SPACTION_INCLUDE_UNARYOPERATOR_H_
