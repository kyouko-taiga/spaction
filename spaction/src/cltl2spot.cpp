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

#include "cltl2spot.h"

#include <iostream>

#include <spot/ltlast/atomic_prop.hh>
#include <spot/ltlast/binop.hh>
#include <spot/ltlast/constant.hh>
#include <spot/ltlast/multop.hh>
#include <spot/ltlast/unop.hh>
#include <spot/ltlast/visitor.hh>
#include <spot/ltlenv/defaultenv.hh>

#include "CltlFormulaVisitor.h"

#include "AtomicProposition.h"
#include "BinaryOperator.h"
#include "ConstantExpression.h"
#include "MultOperator.h"
#include "UnaryOperator.h"
#include "CltlFormulaFactory.h"

#include "Logger.h"


namespace spaction {

class spot_transformer : public CltlFormulaVisitor {
 public:
    /// constructor
    explicit spot_transformer(): result(nullptr) {}
    /// destructor
    ~spot_transformer() { }

    void visit(const std::shared_ptr<AtomicProposition> &formula) final {
        result = spot::ltl::atomic_prop::instance(formula->value(), spot::ltl::default_environment::instance());
    }

    void visit(const std::shared_ptr<ConstantExpression> &formula) final {
        if (formula->value()) {
            result = spot::ltl::constant::true_instance();
        } else {
            result = spot::ltl::constant::false_instance();
        }
    }

    void visit(const std::shared_ptr<UnaryOperator> &formula) final {
        formula->operand()->accept(*this);
        switch (formula->operator_type()) {
            case UnaryOperator::kNot:
                result = spot::ltl::unop::instance(spot::ltl::unop::Not, result);
                break;
            case UnaryOperator::kNext:
                result = spot::ltl::unop::instance(spot::ltl::unop::X, result);
                break;
        }
    }

    void visit(const std::shared_ptr<BinaryOperator> &formula) final {
        formula->left()->accept(*this);
        const spot::ltl::formula *left = result;
        formula->right()->accept(*this);
        const spot::ltl::formula *right = result;

        switch (formula->operator_type()) {
            case BinaryOperator::kUntil:
                result = spot::ltl::binop::instance(spot::ltl::binop::U, left, right);
                break;
            case BinaryOperator::kRelease:
                result = spot::ltl::binop::instance(spot::ltl::binop::R, left, right);
                break;

            default:
                LOG_FATAL << "cost operators are not convertible to spot" << std::endl;
                result = nullptr;
                throw std::runtime_error("cost operators are not convertible to spot");
                break;
        }
    }

    void visit(const std::shared_ptr<MultOperator> &formula) final {
        std::vector<const spot::ltl::formula *> tmp;
        for (auto &c: formula->childs()) {
            c->accept(*this);
            tmp.push_back(result);
        }

        switch (formula->operator_type()) {
            case MultOperator::kOr:
                result = spot::ltl::multop::instance(spot::ltl::multop::Or, &tmp);
                break;
            case MultOperator::kAnd:
                result = spot::ltl::multop::instance(spot::ltl::multop::And, &tmp);
                break;
        }
    }

    const spot::ltl::formula *get() const { return result; }

 private:
    const spot::ltl::formula *result;
};

const spot::ltl::formula *cltl2spot(const CltlFormulaPtr &formula) {
    assert(formula->is_ltl());
    spot_transformer visitor;
    formula->accept(visitor);
    return visitor.get();
}

class cltl_transformer : public spot::ltl::visitor {
 public:
    explicit cltl_transformer(CltlFormulaFactory *f): result(nullptr), _factory(f) {}
    ~cltl_transformer() {}

    void visit(const spot::ltl::atomic_prop* node) override {
        result = _factory->make_atomic(node->name());
    }

    void visit(const spot::ltl::constant* node) override {
        switch (node->val()) {
            case spot::ltl::constant::True:
                result = _factory->make_constant(true);
                break;
            case spot::ltl::constant::False:
                result = _factory->make_constant(false);
                break;
            default:
                std::cerr << "Empty word is not valid for a CLTL formula" << std::endl;
                assert(false);
                break;
        }
    }

    void visit(const spot::ltl::binop* node) override {
        node->first()->accept(*this);
        auto left = result;
        node->second()->accept(*this);
        auto right = result;
        switch (node->op()) {
            case spot::ltl::binop::Implies:
                result = _factory->make_imply(left, right);
                break;
            case spot::ltl::binop::U:
                result = _factory->make_until(left, right);
                break;
            case spot::ltl::binop::R:
                result = _factory->make_release(left, right);
                break;
            default:
                std::cerr << "operator not supported by spot to cltl translation" << std::endl;
                assert(false);
                break;
        }
    }

    void visit(const spot::ltl::unop* node) override {
        node->child()->accept(*this);
        auto child = result;
        switch (node->op()) {
            case spot::ltl::unop::Not:
                result = _factory->make_not(child);
                break;
            case spot::ltl::unop::X:
                result = _factory->make_next(child);
                break;
            case spot::ltl::unop::F:
                result = _factory->make_finally(child);
                break;
            case spot::ltl::unop::G:
                result = _factory->make_globally(child);
                break;
            default:
                std::cerr << "operator not supported by spot to cltl translation" << std::endl;
                assert(false);
                break;
        }
    }

    void visit(const spot::ltl::multop* node) override {
        assert(node->size() > 1);
        std::vector<CltlFormulaPtr> children(node->size(), nullptr);
        for (unsigned i = 0 ; i != node->size() ; ++i) {
            node->nth(i)->accept(*this);
            children[i] = result;
        }
        switch (node->op()) {
            case spot::ltl::multop::Or:
                result = _factory->make_or(children[0], children[1]);
                for (unsigned i = 2 ; i != children.size() ; ++i) {
                    result = _factory->make_or(result, children[i]);
                }
                break;
            case spot::ltl::multop::And:
                result = _factory->make_and(children[0], children[1]);
                for (unsigned i = 2 ; i != children.size() ; ++i) {
                    result = _factory->make_and(result, children[i]);
                }
                break;
            default:
                std::cerr << "operator not supported by spot to cltl translation" << std::endl;
                assert(false);
                break;
        }
    }

    void visit(const spot::ltl::bunop* node) override {
        LOG_FATAL << "bunop not supported, and should not occur in an LTL formula." << std::endl;
        throw std::runtime_error("spot bunop are not supported by spaction, and should not occur in an LTL formula.");
    }

    inline CltlFormulaPtr get() const { return result; }

 private:
    CltlFormulaPtr result;
    CltlFormulaFactory *_factory;
};

CltlFormulaPtr spot2cltl(const spot::ltl::formula *f, CltlFormulaFactory *factory) {
    assert(f->is_ltl_formula());
    cltl_transformer v(factory);
    f->accept(v);
    return v.get();
}

}  // namespace spaction
