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
#include "CltlFormulaVisitor.h"

namespace spaction {

UnaryOperator::UnaryOperator(UnaryOperatorType type, const CltlFormulaPtr &operand) :
    _type(type), _operand(operand) {
}

bool UnaryOperator::operator==(const CltlFormula &rhs) const {
    if(rhs.formula_type() != CltlFormula::kUnaryOperator)
        return false;

    const UnaryOperator &uo = static_cast<const UnaryOperator &>(rhs);
    return (uo._type == _type) and (uo._operand == _operand);
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
