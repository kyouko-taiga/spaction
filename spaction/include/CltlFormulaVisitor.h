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

// corresponds to the class named instant_inf in printer.cc
class InstantiateInf : public CltlFormulaVisitor {
 public:
    explicit InstantiateInf(CltlFormulaFactory *factory) : _factory(factory), _n(0), _result(0) { }

    // corresponds to the function named instantiate_inf in visitor.h
    CltlFormulaPtr operator()(const CltlFormulaPtr &formula, unsigned int n);

    virtual void visit(const std::shared_ptr<AtomicProposition> &formula) final;
    virtual void visit(const std::shared_ptr<ConstantExpression> &formula) final;
    virtual void visit(const std::shared_ptr<UnaryOperator> &formula) final;
    virtual void visit(const std::shared_ptr<BinaryOperator> &formula) final;

 private:
    CltlFormulaFactory *_factory;
    unsigned int _n;
    CltlFormulaPtr _result;
};

// corresponds to the class named instant_sup in printer.cc
class InstantiateSup : public CltlFormulaVisitor {
    explicit InstantiateSup() : _n(0), _result(0) { }

    // corresponds to the function named instantiate_sup in visitor.h
    CltlFormulaPtr operator()(const CltlFormulaPtr &formula, unsigned int n);

    virtual void visit(const std::shared_ptr<AtomicProposition> &formula) final;
    virtual void visit(const std::shared_ptr<ConstantExpression> &formula) final;
    virtual void visit(const std::shared_ptr<UnaryOperator> &formula) final;
    virtual void visit(const std::shared_ptr<BinaryOperator> &formula) final;

 private:
     unsigned int _n;
     const CltlFormulaPtr _result;
};

}  // namespace spaction

#endif  // SPACTION_INCLUDE_CLTLFORMULAVISITOR_H_
