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

#include <functional>

#include "CltlFormulaFactory.h"

#include "AtomicProposition.h"
#include "CltlFormula.h"
#include "ConstantExpression.h"
#include "UnaryOperator.h"
#include "BinaryOperator.h"

namespace spaction {

CltlFormulaPtr CltlFormulaFactory::_make_shared_formula(CltlFormula *formula) {
    // try to find the formula within the unique index and return its shared pointer
    for (auto f : _formulae) {
        if (f->syntactic_eq(*formula)) return f->shared_from_this();
    }

    // insert the new formula in the unique index and creates its shared pointer
    _formulae.insert(formula);
    return CltlFormulaPtr(formula, std::bind(&CltlFormulaFactory::_deleter, this,
                                             std::placeholders::_1));
}

CltlFormulaPtr CltlFormulaFactory::make_atomic(const std::string &value) {
    return _make_shared_formula(new AtomicProposition(value, this));
}

CltlFormulaPtr CltlFormulaFactory::make_constant(bool value) {
    return _make_shared_formula(new ConstantExpression(value, this));
}

CltlFormulaPtr CltlFormulaFactory::make_unary(UnaryOperator::UnaryOperatorType operator_type,
                                              const CltlFormulaPtr &formula) {
    return _make_shared_formula(new UnaryOperator(operator_type, formula, this));
}

CltlFormulaPtr CltlFormulaFactory::make_next(const CltlFormulaPtr &f) {
    return _make_shared_formula(new UnaryOperator(UnaryOperator::kNext, f, this));
}

CltlFormulaPtr CltlFormulaFactory::make_not(const CltlFormulaPtr &f) {
    return _make_shared_formula(new UnaryOperator(UnaryOperator::kNot, f, this));
}

CltlFormulaPtr CltlFormulaFactory::make_binary(BinaryOperator::BinaryOperatorType operator_type,
                                               const CltlFormulaPtr &left,
                                               const CltlFormulaPtr &right) {
    return _make_shared_formula(new BinaryOperator(operator_type, left, right, this));
}

CltlFormulaPtr CltlFormulaFactory::make_or(const CltlFormulaPtr &l, const CltlFormulaPtr &r) {
    return _make_shared_formula(new BinaryOperator(BinaryOperator::kOr, l, r, this));
}

CltlFormulaPtr CltlFormulaFactory::make_and(const CltlFormulaPtr &l, const CltlFormulaPtr &r) {
    return _make_shared_formula(new BinaryOperator(BinaryOperator::kAnd, l, r, this));
}

CltlFormulaPtr CltlFormulaFactory::make_until(const CltlFormulaPtr &l, const CltlFormulaPtr &r) {
    return _make_shared_formula(new BinaryOperator(BinaryOperator::kUntil, l, r, this));
}

CltlFormulaPtr CltlFormulaFactory::make_release(const CltlFormulaPtr &l, const CltlFormulaPtr &r) {
    return _make_shared_formula(new BinaryOperator(BinaryOperator::kRelease, l, r, this));
}

CltlFormulaPtr CltlFormulaFactory::make_costuntil(const CltlFormulaPtr &l,
                                              const CltlFormulaPtr &r) {
    return _make_shared_formula(new BinaryOperator(BinaryOperator::kCostUntil, l, r, this));
}

CltlFormulaPtr CltlFormulaFactory::make_costrelease(const CltlFormulaPtr &l,
                                                const CltlFormulaPtr &r) {
    return _make_shared_formula(new BinaryOperator(BinaryOperator::kCostRelease, l, r, this));
}

CltlFormulaPtr CltlFormulaFactory::make_imply(const CltlFormulaPtr &l, const CltlFormulaPtr &r) {
    const CltlFormulaPtr &lhs = make_not(l);
    return make_or(lhs, r);
}

CltlFormulaPtr CltlFormulaFactory::make_finally(const CltlFormulaPtr &f) {
    const CltlFormulaPtr &ftrue = make_constant(true);
    return make_until(ftrue, f);
}

CltlFormulaPtr CltlFormulaFactory::make_globally(const CltlFormulaPtr &f) {
    const CltlFormulaPtr &ffalse = make_constant(false);
    return make_release(ffalse, f);
}

// FN f = false UN f
CltlFormulaPtr CltlFormulaFactory::make_costfinally(const CltlFormulaPtr &f) {
    const CltlFormulaPtr &ffalse = make_constant(false);
    return make_costuntil(ffalse, f);
}

// GN f = true RN f
CltlFormulaPtr CltlFormulaFactory::make_costglobally(const CltlFormulaPtr &f) {
    const CltlFormulaPtr &ftrue = make_constant(true);
    return make_costrelease(ftrue, f);
}

}  // namespace spaction
