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

#ifndef SPACTION_INCLUDE_INSTANTIATOR_H_
#define SPACTION_INCLUDE_INSTANTIATOR_H_

#include "CltlFormulaVisitor.h"

namespace spaction {

class Instantiator : public CltlFormulaVisitor {
 public:
    explicit Instantiator() : _n(0), _result(0) { }

    /// Instantiate CLTL formula to LTL formulae
    /// for an integer n, u |= f(n) iff (u,n) |= f
    /// @param formula  a CLTL formula to instantiate
    /// @param n        a non-negative integer
    /// @return         g LTL formula s.t. for all word u, u |= g iff (u,n) |= formula
    CltlFormulaPtr operator()(const CltlFormulaPtr &formula, unsigned int n);

    virtual Instantiator *copy() const = 0;

    virtual void visit(const std::shared_ptr<AtomicProposition> &formula) final;
    virtual void visit(const std::shared_ptr<ConstantExpression> &formula) final;
    virtual void visit(const std::shared_ptr<UnaryOperator> &formula) final;
    virtual void visit(const std::shared_ptr<BinaryOperator> &formula) final;

 protected:
    unsigned int _n;
    CltlFormulaPtr _result;

    /// Handles the rewriting of Cost Until formulae.
    /// @remarks
    ///     This class should be implemented to specify the behaviour of the Cost Until operator
    ///     under whether inf or sup instantiation.
    /// @param
    ///     \a left and \a right are assumed to be LTL formulae (already instantiated)
    virtual CltlFormulaPtr _rewrite_cost_until(const CltlFormulaPtr &formula,
                                               const CltlFormulaPtr &left,
                                               const CltlFormulaPtr &right,
                                               Instantiator *instantiator) const = 0;
};

// corresponds to the class named instant_inf in printer.cc
class InstantiateInf : public Instantiator {
 public:
    virtual inline Instantiator *copy() const { return new InstantiateInf(); }

 protected:
    virtual CltlFormulaPtr _rewrite_cost_until(const CltlFormulaPtr &formula,
                                               const CltlFormulaPtr &left,
                                               const CltlFormulaPtr &right,
                                               Instantiator *instantiator) const;
};

// corresponds to the class named instant_sup in printer.cc
// obsolete, DO NOT USE
class InstantiateSup : public Instantiator {
 public:
    virtual inline Instantiator *copy() const { return new InstantiateSup(); }

 protected:
    virtual CltlFormulaPtr _rewrite_cost_until(const CltlFormulaPtr &formula,
                                               const CltlFormulaPtr &left,
                                               const CltlFormulaPtr &right,
                                               Instantiator *instantiator) const;
};

}  // namespace spaction

#endif  // SPACTION_INCLUDE_INSTANTIATOR_H_
