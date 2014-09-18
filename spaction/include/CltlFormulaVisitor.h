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

#ifndef SPACTION_INCLUDE_CLTLFORMULAVISITOR_H_
#define SPACTION_INCLUDE_CLTLFORMULAVISITOR_H_

#include <memory>
#include "CltlFormula.h"

namespace spaction {

class CltlFormula;
class CltlFormulaFactory;

class AtomicProposition;
class ConstantExpression;
class UnaryOperator;
class BinaryOperator;

class CltlFormulaVisitor {
 public:
    virtual ~CltlFormulaVisitor() = 0;

    virtual void visit(const std::shared_ptr<AtomicProposition> &formula) = 0;
    virtual void visit(const std::shared_ptr<ConstantExpression> &formula) = 0;
    virtual void visit(const std::shared_ptr<UnaryOperator> &formula) = 0;
    virtual void visit(const std::shared_ptr<BinaryOperator> &formula) = 0;
};

class Instantiator : public CltlFormulaVisitor {
 public:
    explicit Instantiator(CltlFormulaFactory *factory) : _factory(factory), _n(0), _result(0) { }

    // corresponds to the function named instantiate_inf in visitor.h
    CltlFormulaPtr operator()(const CltlFormulaPtr &formula, unsigned int n);

    virtual Instantiator *copy() const = 0;

    virtual void visit(const std::shared_ptr<AtomicProposition> &formula) final;
    virtual void visit(const std::shared_ptr<ConstantExpression> &formula) final;
    virtual void visit(const std::shared_ptr<UnaryOperator> &formula) final;
    virtual void visit(const std::shared_ptr<BinaryOperator> &formula) final;

 protected:
    CltlFormulaFactory *_factory;
    unsigned int _n;
    CltlFormulaPtr _result;

    /// Handles the rewriting of Cost Until formulae.
    /// @remarks
    ///     This class should be implemented to specify the behaviour of the Cost Until operator
    ///     under whether inf or sup instantiation.
    virtual CltlFormulaPtr _rewrite_cost_until(const CltlFormulaPtr &formula,
                                               const CltlFormulaPtr &left,
                                               const CltlFormulaPtr &right,
                                               Instantiator *instantiator) const = 0;
};

// corresponds to the class named instant_inf in printer.cc
class InstantiateInf : public Instantiator {
 public:
    explicit InstantiateInf(CltlFormulaFactory *factory) : Instantiator(factory) { }

    virtual inline Instantiator *copy() const { return new InstantiateInf(_factory); }

 protected:
    virtual CltlFormulaPtr _rewrite_cost_until(const CltlFormulaPtr &formula,
                                               const CltlFormulaPtr &left,
                                               const CltlFormulaPtr &right,
                                               Instantiator *instantiator) const;
};

// corresponds to the class named instant_sup in printer.cc
class InstantiateSup : public Instantiator {
 public:
    explicit InstantiateSup(CltlFormulaFactory *factory) : Instantiator(factory) { }

    virtual inline Instantiator *copy() const { return new InstantiateSup(_factory); }

 protected:
    virtual CltlFormulaPtr _rewrite_cost_until(const CltlFormulaPtr &formula,
                                               const CltlFormulaPtr &left,
                                               const CltlFormulaPtr &right,
                                               Instantiator *instantiator) const;
};

}  // namespace spaction

#endif  // SPACTION_INCLUDE_CLTLFORMULAVISITOR_H_
