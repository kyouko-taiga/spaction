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

#include "CltlFormula.h"

namespace spaction {

class CltlFormulaFactory;
class CltlFormulaVisitor;

class BinaryOperator : public CltlFormula {
 public:
    enum BinaryOperatorType : char {
        kOr,
        kAnd,
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

    /// Returns true if `rhs` is a BinaryOperator whose operator and operands are equal to ours.
    virtual bool operator==(const CltlFormula &rhs) const;

    inline const CltlFormulaPtr &left() const { return _left; }
    inline const CltlFormulaPtr &right() const { return _right; }

    void accept(CltlFormulaVisitor &visitor) override;

    std::string dump() const override;

 protected:
    explicit BinaryOperator(BinaryOperatorType type, const CltlFormulaPtr &left,
                            const CltlFormulaPtr &right);
    ~BinaryOperator();

 private:
    friend class CltlFormulaFactory;

    BinaryOperatorType _type;
    const CltlFormulaPtr _left;
    const CltlFormulaPtr _right;
};

}  // namespace spaction

#endif  // SPACTION_INCLUDE_BINARYOPERATOR_H_
