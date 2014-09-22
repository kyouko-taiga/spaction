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

#include "UnaryOperator.h"

#include "BinaryOperator.h"
#include "CltlFormulaFactory.h"
#include "CltlFormulaVisitor.h"

namespace spaction {

UnaryOperator::UnaryOperator(UnaryOperatorType type, const CltlFormulaPtr &operand,
                             CltlFormulaFactory *creator) :
    CltlFormula(creator), _type(type), _operand(operand) {
}

bool UnaryOperator::operator==(const CltlFormula &rhs) const {
    if(rhs.formula_type() != CltlFormula::kUnaryOperator)
        return false;

    const UnaryOperator &uo = static_cast<const UnaryOperator &>(rhs);
    return (uo._type == _type) and (uo._operand == _operand);
}

bool UnaryOperator::operator<(const CltlFormula &rhs) const {
    switch (rhs.formula_type()) {
        case CltlFormula::kConstantExpression:
        case CltlFormula::kAtomicProposition:
            return false;
        case CltlFormula::kUnaryOperator: {
            const UnaryOperator &uo = static_cast<const UnaryOperator &>(rhs);
            return (_type != uo._type) ? (_type < uo._type) : (_operand < uo._operand);
        }
        case CltlFormula::kBinaryOperator:
            return true;
    }
}

CltlFormulaPtr UnaryOperator::to_nnf() {
    // get the nnf of the operand
    CltlFormulaPtr &&nnf_operand = _operand->to_nnf();

    // if our operator is !, push it to the leaves
    if (_type == UnaryOperator::kNot) {
        // if the operand is an unary operator
        if (nnf_operand->formula_type() == CltlFormula::kUnaryOperator) {
            UnaryOperator *uo = static_cast<UnaryOperator*>(nnf_operand.get());
            switch (uo->operator_type()) {
                case UnaryOperator::kNot:
                    return nnf_operand;
                case UnaryOperator::kNext:
                    return _creator->make_next(_creator->make_not(uo->operand()));
            }
        }

        // if the operand is a binary operator
        if (nnf_operand->formula_type() == CltlFormula::kBinaryOperator) {
            BinaryOperator *bo = static_cast<BinaryOperator*>(nnf_operand.get());
            const CltlFormulaPtr &left = bo->left()->to_nnf();
            const CltlFormulaPtr &right = bo->right()->to_nnf();

            switch (bo->operator_type()) {
                case BinaryOperator::kOr:
                    return _creator->make_and(left, right);
                case BinaryOperator::kAnd:
                    return _creator->make_or(left, right);
                case BinaryOperator::kUntil:
                    return _creator->make_release(left, right);
                case BinaryOperator::kRelease:
                    return _creator->make_until(left, right);
                case BinaryOperator::kCostUntil:
                    return _creator->make_costrelease(left, right);
                case BinaryOperator::kCostRelease:
                    return _creator->make_costuntil(left, right);
            }
        }
    }

    // simply return negation of the operand in nnf
    return _creator->make_not(_operand);
}

void UnaryOperator::accept(CltlFormulaVisitor &visitor) {
    // explicitly cast shared_from_this to the a derived class shared_ptr
    visitor.visit(std::dynamic_pointer_cast<UnaryOperator>(shared_from_this()));
}

std::string UnaryOperator::dump() const {
    std::string result;
    switch (_type) {
        case kNext:
            result = "X";
            break;
        case kNot:
            result = "!";
            break;
    }
    result += " (";
    result += _operand->dump();
    result += ")";
    return result;
}

}  // namespace spaction
