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

#include "BinaryOperator.h"

#include <unordered_set>
#include <stack>

#include "CltlFormulaFactory.h"
#include "CltlFormulaVisitor.h"

namespace spaction {
    
BinaryOperator::BinaryOperator(BinaryOperatorType type, const CltlFormulaPtr &left,
                               const CltlFormulaPtr &right, CltlFormulaFactory *creator) :
    CltlFormula(creator), _type(type), _left(left), _right(right) {
}

bool BinaryOperator::syntactic_eq(const CltlFormula &rhs) const {
    if (rhs.formula_type() != CltlFormula::kBinaryOperator)
        return false;

    const BinaryOperator &bo = static_cast<const BinaryOperator &>(rhs);
    if (bo._type != _type)
        return false;

    switch (bo.operator_type()) {
        case BinaryOperator::kOr:
        case BinaryOperator::kAnd:
            return _leaves() == bo._leaves();
        case BinaryOperator::kUntil:
        case BinaryOperator::kRelease:
        case BinaryOperator::kCostUntil:
        case BinaryOperator::kCostRelease:
            return ((bo._left == _left) and (bo._right == _right));
    }
}

CltlFormulaPtr BinaryOperator::to_nnf() {
    return _creator->make_binary(_type, _left->to_nnf(), _right->to_nnf());
}

CltlFormulaPtr BinaryOperator::to_dnf() {
    // get the negative normal form of itself
    CltlFormulaPtr &&nnf_self = this->to_nnf();

    // check if `nnf_self` is a binary operator
    if (nnf_self->formula_type() == CltlFormula::kBinaryOperator) {
        // recursively transform operands
        BinaryOperator *bo_self = static_cast<BinaryOperator*>(nnf_self.get());
        const CltlFormulaPtr &left = bo_self->left()->to_dnf();
        const CltlFormulaPtr &right = bo_self->right()->to_dnf();

        // check if `nnf_self` is of the form (x && y)
        if (bo_self->operator_type() == BinaryOperator::kAnd) {
            if (right->formula_type() == CltlFormula::kBinaryOperator) {
                BinaryOperator *bo_right = static_cast<BinaryOperator*>(right.get());
                if (bo_right->operator_type() == BinaryOperator::kOr) {
                    // distribute a * (b + c)
                    const CltlFormulaPtr &a = _creator->make_and(left, bo_right->left());
                    const CltlFormulaPtr &b = _creator->make_and(left, bo_right->right());
                    return _creator->make_or(a, b)->to_dnf();
                }
            } else if (left->formula_type() == CltlFormula::kBinaryOperator) {
                BinaryOperator *bo_left = static_cast<BinaryOperator*>(left.get());
                if (bo_left->operator_type() == BinaryOperator::kOr) {
                    // distribute (a + b) * c
                    const CltlFormulaPtr &a = _creator->make_and(right, bo_left->left());
                    const CltlFormulaPtr &b = _creator->make_and(right, bo_left->right());
                    return _creator->make_or(a, b)->to_dnf();
                }
            }
        } else {
            // returns a binary operator with transformed operands
            return _creator->make_binary(bo_self->operator_type(), left, right);
        }
    }

    // no further transformation to be performed since `nnf_self` is not a binary operator
    return nnf_self;
}

std::unordered_set<CltlFormula*> BinaryOperator::_leaves() const {
    std::unordered_set<CltlFormula*> leaves;
    std::stack<const BinaryOperator*> stack({this});
    bool is_leaf;

    // unfold the operator such that we get the set of leaves
    while (!stack.empty()) {
        const BinaryOperator *current = stack.top();
        stack.pop();

        // push the right member on the stack, unless it is not a compatible operation
        is_leaf = true;
        if (current->right()->formula_type() == CltlFormula::kBinaryOperator) {
            BinaryOperator *bo = static_cast<BinaryOperator*>(current->right().get());
            if (bo->operator_type() == _type) {
                stack.push(bo);
                is_leaf = false;
            }
        }
        if (is_leaf) {
            leaves.insert(current->right().get());
        }

        // push the left member on the stack, unless it is not a compatible operation
        is_leaf = true;
        if (current->left()->formula_type() == CltlFormula::kBinaryOperator) {
            BinaryOperator *bo = static_cast<BinaryOperator*>(current->left().get());
            if (bo->operator_type() == _type) {
                stack.push(bo);
                is_leaf = false;
            }
        }
        if (is_leaf) {
            leaves.insert(current->left().get());
        }
    }

    return leaves;
}

void BinaryOperator::accept(CltlFormulaVisitor &visitor) {
    // explicitly cast shared_from_this to the a derived class shared_ptr
    visitor.visit(std::dynamic_pointer_cast<BinaryOperator>(shared_from_this()));
}
    
std::string BinaryOperator::dump() const {
    std::string result;
    result += "(";
    result += _left->dump();
    result += ") ";
    switch (_type) {
        case kOr:
            result += "||";
            break;
        case kAnd:
            result += "&&";
            break;
        case kUntil:
            result += "U";
            break;
        case kRelease:
            result += "R";
            break;
        case kCostUntil:
            result += "UN";
            break;
        case kCostRelease:
            result += "RN";
            break;
    }
    result += " (";
    result += _right->dump();
    result += ")";
    return result;
}

}  // namespace spaction
