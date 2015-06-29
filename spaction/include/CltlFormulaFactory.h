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

#ifndef SPACTION_INCLUDE_CLTLFORMULAFACTORY_H_
#define SPACTION_INCLUDE_CLTLFORMULAFACTORY_H_

#include <unordered_set>

#include "BinaryOperator.h"
#include "CltlFormula.h"
#include "MultOperator.h"
#include "UnaryOperator.h"

namespace spaction {

/// A factory class for Cost LTL formulae.
class CltlFormulaFactory {
 public:
    CltlFormulaPtr make_atomic(const std::string &value);
    CltlFormulaPtr make_constant(bool value);

    CltlFormulaPtr make_unary(UnaryOperator::UnaryOperatorType operator_type,
                              const CltlFormulaPtr &formula);
    CltlFormulaPtr make_next(const CltlFormulaPtr &formula);
    CltlFormulaPtr make_not(const CltlFormulaPtr &formula);

    CltlFormulaPtr make_binary(BinaryOperator::BinaryOperatorType operator_type,
                               const CltlFormulaPtr &left, const CltlFormulaPtr &right);
    CltlFormulaPtr make_nary(MultOperator::MultOperatorType operator_type,
                             const std::vector<CltlFormulaPtr> &ops);
    CltlFormulaPtr make_nary(MultOperator::MultOperatorType operator_type,
                             const CltlFormulaPtr &left, const CltlFormulaPtr &right);
    CltlFormulaPtr make_or(const CltlFormulaPtr &left, const CltlFormulaPtr &right);
    CltlFormulaPtr make_or(const std::vector<CltlFormulaPtr> &ops);
    CltlFormulaPtr make_and(const CltlFormulaPtr &left, const CltlFormulaPtr &right);
    CltlFormulaPtr make_and(const std::vector<CltlFormulaPtr> &ops);
    CltlFormulaPtr make_until(const CltlFormulaPtr &left, const CltlFormulaPtr &right);
    CltlFormulaPtr make_release(const CltlFormulaPtr &left, const CltlFormulaPtr &right);
    CltlFormulaPtr make_costuntil(const CltlFormulaPtr &left, const CltlFormulaPtr &right);
    CltlFormulaPtr make_costrelease(const CltlFormulaPtr &left, const CltlFormulaPtr &right);

    /// Builds a formula semantically equivalent to "`left` implies `right`".
    CltlFormulaPtr make_imply(const CltlFormulaPtr &left, const CltlFormulaPtr &right);
    /// Builds a formula semantically equivalent to "Globally `formula`".
    CltlFormulaPtr make_globally(const CltlFormulaPtr &formula);
    /// Builds a formula semantically equivalent to "Finally `formula`".
    CltlFormulaPtr make_finally(const CltlFormulaPtr &formula);
    /// Builds a formula semantically equivalent to "Finally^N `formula`".
    CltlFormulaPtr make_costfinally(const CltlFormulaPtr &formula);
    /// Builds a formula semantically equivalent to "Globally^N `formula`".
    CltlFormulaPtr make_costglobally(const CltlFormulaPtr &formula);

 private:
    /// Stores the unique index.
    std::unordered_set<CltlFormula*> _formulae;

    CltlFormulaPtr _make_shared_formula(CltlFormula *formula);

    /// Removes a formula from the unique index once it is no more referenced.
    /// @remarks
    ///     This custom deleter is bound to the shared pointers built by this factory. It gets
    ///     called when the references counter of a particular shared pointer reaches 0.
    void _deleter(CltlFormula *formula) {
        _formulae.erase(formula);
        delete formula;
    }
};

}  // namespace spaction

#endif  // SPACTION_INCLUDE_CLTLFORMULAFACTORY_H_
