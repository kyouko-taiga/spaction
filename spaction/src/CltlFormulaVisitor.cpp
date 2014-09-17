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

#include "CltlFormulaVisitor.h"

#include <stdexcept>

#include "AtomicProposition.h"
#include "CltlFormula.h"
#include "CltlFormulaFactory.h"
#include "ConstantExpression.h"
#include "BinaryOperator.h"
#include "UnaryOperator.h"

namespace spaction {

CltlFormulaPtr InstantiateInf::operator()(const CltlFormulaPtr &formula, unsigned int n) {
    _n = n;
    formula->accept(*this);
    return _result;
}

void InstantiateInf::visit(const std::shared_ptr<AtomicProposition> &formula) {
    // _result = std::dynamic_pointer_cast<const CltlFormula>(formula);
    _result = formula;
}

void InstantiateInf::visit(const std::shared_ptr<ConstantExpression> &formula) {
    _result = formula;
}

void InstantiateInf::visit(const std::shared_ptr<UnaryOperator> &formula) {
    CltlFormulaPtr operand = InstantiateInf(_factory)(formula.get()->operand(), _n);
    _result = _factory->make_unary(formula.get()->operator_type(), operand);
}

void InstantiateInf::visit(const std::shared_ptr<BinaryOperator> &formula) {
    InstantiateInf instantiator(_factory);
    CltlFormulaPtr left = instantiator(formula.get()->left(), _n);
    CltlFormulaPtr right = instantiator(formula.get()->right(), _n);

    switch (formula->operator_type()) {
        case BinaryOperator::kOr:
        case BinaryOperator::kAnd:
        case BinaryOperator::kUntil:
        case BinaryOperator::kRelease:
            _result = _factory->make_binary(formula.get()->operator_type(), left, right);
            break;
        case BinaryOperator::kCostUntil:
            // \todo ...
            break;
        case BinaryOperator::kCostRelease:
            // \todo but should not happen here
            throw std::domain_error("shouldn't encounter cost release during inf instantiation");
            break;
    }
}
    
}  // namespace spaction
