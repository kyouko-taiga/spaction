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
#include "CltlFormulaVisitor.h"

namespace spaction {
    
BinaryOperator::BinaryOperator(BinaryOperatorType type, const CltlFormulaPtr &left,
                               const CltlFormulaPtr &right, CltlFormulaFactory *creator) :
    CltlFormula(creator), _type(type), _left(left), _right(right) {
}

bool BinaryOperator::operator==(const CltlFormula &rhs) const {
    if (rhs.formula_type() != CltlFormula::kBinaryOperator)
        return false;
    
    const BinaryOperator &bo = static_cast<const BinaryOperator &>(rhs);
    if (bo._type != _type)
        return false;

    switch (bo.operator_type()) {
        case BinaryOperator::kOr:
        case BinaryOperator::kAnd: {
            // check for commutativity
            bool a = ((bo._left == _left) and (bo._right == _right));
            bool b = ((bo._right == _left) and (bo._left == _right));
            return a or b;
        }

        case BinaryOperator::kUntil:
        case BinaryOperator::kRelease:
        case BinaryOperator::kCostUntil:
        case BinaryOperator::kCostRelease:
            return ((bo._left == _left) and (bo._right == _right));
    }
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
