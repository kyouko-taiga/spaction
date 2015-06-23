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

bool UnaryOperator::syntactic_eq(const CltlFormula &rhs) const {
    if (rhs.formula_type() != CltlFormula::kUnaryOperator)
        return false;

    const UnaryOperator &uo = static_cast<const UnaryOperator &>(rhs);
    return (uo._type == _type) and (uo._operand == _operand);
}

CltlFormulaPtr UnaryOperator::to_nnf() {
    // if the top-level operator is NOT, push it to the leaves
    if (_type == UnaryOperator::kNot) {
        // if the operand is itself a unary operation
        if (_operand->formula_type() == CltlFormula::kUnaryOperator) {
            UnaryOperator *uo = static_cast<UnaryOperator*>(_operand.get());
            // two NOTs cancel out
            if (uo->operator_type() == UnaryOperator::kNot) {
                return uo->operand()->to_nnf();
            } else {
                return _creator->make_next(_creator->make_not(uo->operand())->to_nnf());
            }
        }

        // if the operand is a binary operation
        if (_operand->formula_type() == CltlFormula::kBinaryOperator) {
            BinaryOperator *bo = static_cast<BinaryOperator*>(_operand.get());
            switch (bo->operator_type()) {
                case BinaryOperator::kOr:
                    return _creator->make_and(bo->left(), bo->right())->to_nnf();
                case BinaryOperator::kAnd:
                    return _creator->make_or(bo->left(), bo->right())->to_nnf();
                case BinaryOperator::kUntil:
                    return _creator->make_release(bo->left(), bo->right())->to_nnf();
                case BinaryOperator::kRelease:
                    return _creator->make_until(bo->left(), bo->right())->to_nnf();
                case BinaryOperator::kCostUntil:
                    return _creator->make_costrelease(bo->left(), bo->right())->to_nnf();
                case BinaryOperator::kCostRelease:
                    return _creator->make_costuntil(bo->left(), bo->right())->to_nnf();
            }
        }

        // in any other cases (i.e. Constant and Atomic Proposition)
        return _creator->make_not(_operand);
    }

    // if the top-level operator is NEXT, just recursive call
    return _creator->make_next(_operand->to_nnf());
}

bool UnaryOperator::is_infltl() const {
    if (_type == kNot) {
        return _operand->is_supltl();
    } else {
        return _operand->is_infltl();
    }
}

bool UnaryOperator::is_supltl() const {
    if (_type == kNot) {
        return _operand->is_infltl();
    } else {
        return _operand->is_supltl();
    }
}

bool UnaryOperator::is_propositional() const {
    if (_type == kNext)
        return false;
    else
        return _operand->is_propositional();
}

bool UnaryOperator::is_nnf() const {
    if (_type == kNot) {
        return _operand->formula_type() == CltlFormula::kAtomicProposition
            or _operand->formula_type() == CltlFormula::kConstantExpression;
    } else {
        return _operand->is_nnf();
    }
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
