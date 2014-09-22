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

#include "ConstantExpression.h"
#include "CltlFormulaVisitor.h"

namespace spaction {

ConstantExpression::ConstantExpression(bool value, CltlFormulaFactory *creator) :
    CltlFormula(creator), _value(value) {
}

bool ConstantExpression::operator==(const CltlFormula &rhs) const {
    if(rhs.formula_type() != CltlFormula::kConstantExpression)
        return false;
    
    const ConstantExpression &ce = static_cast<const ConstantExpression &>(rhs);
    return ce._value == _value;
}

bool ConstantExpression::operator<(const CltlFormula &rhs) const {
    switch (rhs.formula_type()) {
        case CltlFormula::kConstantExpression: {
            const ConstantExpression &ce = static_cast<const ConstantExpression &>(rhs);
            return (!_value and ce._value);
        }
        default:
            return true;
    }
}

void ConstantExpression::accept(CltlFormulaVisitor &visitor) {
    // explicitly cast shared_from_this to the a derived class shared_ptr
    visitor.visit(std::dynamic_pointer_cast<ConstantExpression>(shared_from_this()));
}

std::string ConstantExpression::dump() const {
    return _value ? "true" : "false";
}
    
}  // namespace spaction
