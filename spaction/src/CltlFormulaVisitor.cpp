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

CltlFormulaPtr Instantiator::operator()(const CltlFormulaPtr &formula, unsigned int n) {
    _n = n;
    formula->accept(*this);
    return _result;
}

void Instantiator::visit(const std::shared_ptr<AtomicProposition> &formula) {
    // _result = std::dynamic_pointer_cast<const CltlFormula>(formula);
    _result = formula;
}

void Instantiator::visit(const std::shared_ptr<ConstantExpression> &formula) {
    _result = formula;
}

void Instantiator::visit(const std::shared_ptr<UnaryOperator> &formula) {
    // use a copy of the current instantiator to instantiate operand
    Instantiator *instantiator = this->copy();
    const CltlFormulaPtr &operand = (*instantiator)(formula.get()->operand(), _n);
    delete instantiator;

    _result = _factory->make_unary(formula.get()->operator_type(), operand);
}

void Instantiator::visit(const std::shared_ptr<BinaryOperator> &formula) {
    // use a copy of the current instantiator to instantiate operands
    Instantiator *instantiator = this->copy();
    const CltlFormulaPtr &left = (*instantiator)(formula.get()->left(), _n);
    const CltlFormulaPtr &right = (*instantiator)(formula.get()->right(), _n);

    switch (formula->operator_type()) {
        case BinaryOperator::kOr:
        case BinaryOperator::kAnd:
        case BinaryOperator::kUntil:
        case BinaryOperator::kRelease:
            _result = _factory->make_binary(formula.get()->operator_type(), left, right);
            break;
        case BinaryOperator::kCostUntil:
            _result = _rewrite_cost_until(formula, left, right, instantiator);
            break;
        case BinaryOperator::kCostRelease:
            // \todo but should not happen here
            throw std::domain_error("shouldn't encounter cost release during inf instantiation");
            break;
    }

    delete instantiator;
}

CltlFormulaPtr InstantiateInf::_rewrite_cost_until(const CltlFormulaPtr &formula,
                                                   const CltlFormulaPtr &left,
                                                   const CltlFormulaPtr &right,
                                                   Instantiator *instantiator) const {
    if (_n == 0) {
        return _factory->make_until(left, right);
    }

    const CltlFormulaPtr &tmp_left_1 = _factory->make_until(left, right);
    const CltlFormulaPtr &tmp_left_2 = _factory->make_not(left);
    const CltlFormulaPtr &tmp_rslt_3 = (*instantiator)(formula, _n-1);
    const CltlFormulaPtr &tmp_rght_2 = _factory->make_next(tmp_rslt_3);
    const CltlFormulaPtr &tmp_rslt_2 = _factory->make_and(tmp_left_2, tmp_rght_2);
    const CltlFormulaPtr &tmp_rght_1 = _factory->make_until(left, tmp_rslt_2);
    return _factory->make_or(tmp_left_1, tmp_rght_1);
}

CltlFormulaPtr InstantiateSup::_rewrite_cost_until(const CltlFormulaPtr &formula,
                                                   const CltlFormulaPtr &left,
                                                   const CltlFormulaPtr &right,
                                                   Instantiator *instantiator) const {
    // a U{n=0} b -> true U b
    // doubtful: a U{n}   b -> a U (!a && X (a U{n-1} b))
    // a U{n}   b -> (a && !b) U (!a && !b && X (a U{n-1} b))
    if (_n == 0) {
        CltlFormulaPtr &&ftrue = _factory->make_constant(true);
        return _factory->make_until(ftrue, right);
    }

    const CltlFormulaPtr &lur = (*instantiator)(formula, _n-1);
    const CltlFormulaPtr &x_lur = _factory->make_next(lur);
    const CltlFormulaPtr &nl = _factory->make_not(left);
    const CltlFormulaPtr &nr = _factory->make_not(right);
    const CltlFormulaPtr &nl_and_nr = _factory->make_and(nl, nr);
    const CltlFormulaPtr &nl_and_nr_and_x_lur = _factory->make_and(nl_and_nr, x_lur);
    const CltlFormulaPtr &l_and_nr = _factory->make_and(left, nr);
    return _factory->make_until(l_and_nr, nl_and_nr_and_x_lur);
}

}  // namespace spaction
