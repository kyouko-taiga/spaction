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

#include "Instantiator.h"

#include <stdexcept>

#include "AtomicProposition.h"
#include "CltlFormula.h"
#include "CltlFormulaFactory.h"
#include "ConstantExpression.h"
#include "BinaryOperator.h"
#include "UnaryOperator.h"

namespace spaction {

CltlFormulaPtr Instantiator::operator()(const CltlFormulaPtr &formula, unsigned int n) {
    _preprocess(formula, n);
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
    const CltlFormulaPtr &operand = (*instantiator)(formula->operand(), _n);
    delete instantiator;

    CltlFormulaFactory *factory = formula->creator();
    _result = factory->make_unary(formula->operator_type(), operand);
}

void Instantiator::visit(const std::shared_ptr<BinaryOperator> &formula) {
    // use a copy of the current instantiator to instantiate operands
    Instantiator *instantiator = this->copy();
    const CltlFormulaPtr &left = (*instantiator)(formula->left(), _n);
    const CltlFormulaPtr &right = (*instantiator)(formula->right(), _n);
    CltlFormulaFactory *factory = formula->creator();

    switch (formula->operator_type()) {
        case BinaryOperator::kUntil:
        case BinaryOperator::kRelease:
            // for every boolean binary op o, (f o g)[n] = f[n] o f[n]
            _result = factory->make_binary(formula->operator_type(), left, right);
            break;
        case BinaryOperator::kCostUntil:
            // (f UN g)[n] = (f[n] UN g[n])[n]
            _result = _rewrite_cost_until(formula, left, right, instantiator);
            break;
        case BinaryOperator::kCostRelease:
            // (f RN g)[n] = (f[n] RN g[n])[n]
            _result = _rewrite_cost_release(formula, left, right, instantiator);
            break;
    }

    delete instantiator;
}

void Instantiator::visit(const std::shared_ptr<MultOperator> &formula) {
    // use a copy of the current instantiator to instantiate operands
    Instantiator *instantiator = this->copy();
    std::vector<CltlFormulaPtr> tmp;
    for (auto &c: formula->childs()) {
        tmp.push_back((*instantiator)(c, _n));
    }
    _result = formula->creator()->make_and(tmp);
    delete instantiator;
}

// recall that left and right are assumed to be LTL (already instantiated)
CltlFormulaPtr InstantiateInf::_rewrite_cost_until(const CltlFormulaPtr &formula,
                                                   const CltlFormulaPtr &left,
                                                   const CltlFormulaPtr &right,
                                                   Instantiator *instantiator) const {
    // the formula factory
    CltlFormulaFactory *factory = formula->creator();

    // if f and g are LTL, then (f UN g)[0] = f U g
    if (_n == 0) {
        return factory->make_until(left, right);
    }

    // if f and g are LTL and n > 0 then
    // (f UN g)[n] = (f || X(f UN g)[n-1]) U g

    // @todo    consider using the following semantically equivalent:
    //              f U (f || (!f && X(f U g)[n-1]))
    //          more complicated, but it might produce more deterministic automata

    // recursive call formula[n-1]
    const CltlFormulaPtr &rec_formula = (*instantiator)(formula, _n-1);
    // X(formula[n-1])
    const CltlFormulaPtr &next_rec_formula = factory->make_next(rec_formula);
    // left || X(formula[n-1])
    const CltlFormulaPtr &or_formula = factory->make_or(left, next_rec_formula);
    // (left || X(formula[n-1])) U right
    return factory->make_until(or_formula, right);
}

CltlFormulaPtr InstantiateInf::_rewrite_cost_release(const CltlFormulaPtr &formula,
                                                     const CltlFormulaPtr &left,
                                                     const CltlFormulaPtr &right,
                                                     Instantiator *instantiator) const {
    throw std::domain_error("Cost Release encountered: inf instantiation should be applied to CLTL[<=] formulae only");
}

void InstantiateInf::_preprocess(const CltlFormulaPtr &formula, unsigned int n) const {
    if (! formula->is_infltl())
        throw std::domain_error("ERROR: inf instantiation should be applied to CLTL[<=] formulae only");
}

CltlFormulaPtr InstantiateSup::_rewrite_cost_until(const CltlFormulaPtr &formula,
                                                   const CltlFormulaPtr &left,
                                                   const CltlFormulaPtr &right,
                                                   Instantiator *instantiator) const {
    throw std::domain_error("Cost Until encountered: sup instantiation should be applied to CLTL[>] formulae only");
}

CltlFormulaPtr InstantiateSup::_rewrite_cost_release(const CltlFormulaPtr &formula,
                                                     const CltlFormulaPtr &left,
                                                     const CltlFormulaPtr &right,
                                                     spaction::Instantiator *instantiator) const {
    // the formula factory
    CltlFormulaFactory *factory = formula->creator();

    // \todo double check the following equations:
    // if f and g are LTL, then (f RN g)[0] = f R g
    if (_n == 0) {
        return factory->make_release(left, right);
    }

    // if f and g are LTL and n > 0 then
    // (f RN g)[n] = (f && X(f RN g)[n-1]) R g

    // recursive call formula[n-1]
    const CltlFormulaPtr &rec_formula = (*instantiator)(formula, _n-1);
    // X(formula[n-1])
    const CltlFormulaPtr &next_rec_formula = factory->make_next(rec_formula);
    // left && X(formula[n-1])
    const CltlFormulaPtr &and_formula = factory->make_and(left, next_rec_formula);
    // (left && X(formula[n-1])) R right
    return factory->make_release(and_formula, right);
}

void InstantiateSup::_preprocess(const CltlFormulaPtr &formula, unsigned int n) const {
    if (! formula->is_supltl())
        throw std::domain_error("ERROR: sup instantiation should be applied to CLTL[>] formulae only");
}

}  // namespace spaction
