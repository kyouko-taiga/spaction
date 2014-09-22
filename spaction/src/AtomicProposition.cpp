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

#include "AtomicProposition.h"
#include "CltlFormulaVisitor.h"

namespace spaction {

AtomicProposition::AtomicProposition(const std::string &value, CltlFormulaFactory *creator) :
    CltlFormula(creator), _value(value) {
}

bool AtomicProposition::operator==(const CltlFormula &rhs) const {
    if (rhs.formula_type() != CltlFormula::kAtomicProposition)
        return false;

    const AtomicProposition &ap = static_cast<const AtomicProposition &>(rhs);
    return ap._value == _value;
}

bool AtomicProposition::operator<(const CltlFormula &rhs) const {
    switch (rhs.formula_type()) {
        case CltlFormula::kConstantExpression:
            return false;
        case CltlFormula::kAtomicProposition: {
            const AtomicProposition &ap = static_cast<const AtomicProposition &>(rhs);
            return _hash() < ap._hash();
        }
        default:
            return true;
    }
}

void AtomicProposition::accept(CltlFormulaVisitor &visitor) {
    // explicitly cast shared_from_this to the a derived class shared_ptr
    visitor.visit(std::dynamic_pointer_cast<AtomicProposition>(shared_from_this()));
}

std::string AtomicProposition::dump() const {
    return "\"" + _value + "\"";
}

unsigned int AtomicProposition::_hash() const {
	unsigned int h = 0;
	for (std::size_t i = 0; i < _value.length(); i++) {
		h += _value[i] << (8 * i);
	}
	return h;
}

}  // namespace spaction
