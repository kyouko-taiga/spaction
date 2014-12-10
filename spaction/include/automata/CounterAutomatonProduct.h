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

#ifndef SPACTION_INCLUDE_AUTOMATA_COUNTERAUTOMATONPRODUCT_H_
#define SPACTION_INCLUDE_AUTOMATA_COUNTERAUTOMATONPRODUCT_H_

#include "automata/CounterAutomaton.h"
#include "automata/TransitionSystemProduct.h"

namespace spaction {
namespace automata {

/// @todo Use these explanations to rename all the convoluted stuff below
/// CAProd<Q1, L1, Q2, L2, ALabelProd>      derives from        CA<pair<Q1, Q2>, ALabelProd<L1, L2>::product_type, TSProd<Q1, CounterLabel<L1>, Q2, CounterLabel<L2>, CLWrapper<ALabelProd>::type>>
/// TSProd<Q1, S1, Q2, S2, TSLabelProd>     derives from        TS<pair<Q1, Q2>, TSLabelProd<S1, S2>::product_type>
///
///     CAProd<Q1, L1, Q2, L2, ALabelProd> is built from
/// CA<Q1, L1>                      and     CA<Q2, L2>
///     The underlying TSProd is thus built from
/// TS<Q1, CounterLabel<L1>>        and     TS<Q2, CounterLabel<L2>>
///
///     The product TS underlying CAProd<Q1, L1, Q2, L2, ALabelProd> is
/// TSProd<Q1, CounterLabel<L1>, Q2, CounterLabel<L2>, CLWrapper<ALabelProd>::type>
///     which derives from
/// TS<pair<Q1, Q2>, CLWrapper<ALabelProd>::type<CounterLabel<L1>, CounterLabel<L2>>::product_type>
///     and should derive from
/// TS<pair<Q1,Q2>, CounterLabel<ALabelProd<L1, L2>::product_type>>
///
///     (1) Therefore, we want the following equality to hold
/// CLWrapper<ALabelProd>::type<CounterLabel<L1>, CounterLabel<L2>>::product_type
///     =
/// CounterLabel<ALabelProd<L1, L2>::product_type>
///
///     (2) ALabelProd<L1, L2> should have a type member `product_type`
///     (3) CLWrapper<ALabelProd> should have a template member `type`
///     (4) CLWrapper<ALabelProd>::type<CounterLabel<L1>, CounterLabel<L2>> should have a type member `product_type`
///     (5) CLWrapper<ALabelProd>::type<CounterLabel<L1>, CounterLabel<L2>> should derive from ILabelProd<CounterLabel<L1>, CounterLabel<L2>, CounterLabel<ALabelProd<L1, L2>::product_type>>
///     (6) We also need a template TSP to give to the original CA, such that
///         TSP<pair<Q1, Q2>, CLWrapper<ALabelProd>::type<CounterLabel<L1>, CounterLabel<L2>>::product_type> = TSProd<Q1, CounterLabel<L1>, Q2, CounterLabel<L2>, CLWrapper<ALabelProd>::type>

/// @remark To define a product automaton with your custom labels L1 and L2:
///         Determine what label type to used in the product automata, say P;
///         Declare a template class MyLabelProd with two template arguments;
///         Define/Specialize so that MyLabelProd<L1, L2> implements IAutLabelProd<L1, L2, P>;
///         Simply use CounterAutomatonProduct<Q1, L1, Q2, L2, MyLabelProd>.

/// The interface for LabelProduct
template<typename L, typename R, typename P>
class IAutLabelProd {
public:
    typedef L lhs_type;
    typedef R rhs_type;
    typedef P product_type;

    virtual ~IAutLabelProd() { }

    virtual product_type build(const lhs_type &, const rhs_type &) const = 0;
    virtual lhs_type lhs(const product_type &) const = 0;
    virtual rhs_type rhs(const product_type &) const = 0;
};

/// A helper struct to shorten labels type names
/// @param L1               the lhs label type
/// @param L2               the rhs label type
/// @param LabelProduct     the label product, must implement IAutLabelProd<L1, L2, P> for some P.
template<typename L1, typename L2, template<typename, typename> class LabelProduct>
struct ALabel {
    /// the product label type (in the counter automaton)
    using autprod_label = typename LabelProduct<L1, L2>::product_type;
    /// the product label type wrapped in a CounterLabel (for the underlying TS)
    using tsprod_label = CounterLabel<autprod_label>;

    /// Ensure that given LabelProduct implements ILabelProd.
    static_assert(std::is_base_of<IAutLabelProd<L1, L2, autprod_label>, LabelProduct<L1, L2>>(),
              "In ALabel: the template argument LabelProduct does not implement IAutLabelProduct.");
};

/// A trivial implementation of ILabelProd, serves as a general template for further specialization.
template<typename L1, typename L2, typename P, template<typename, typename> class LabelProduct>
class TSLabelProdImpl {};

/// A specialization that handles the CounterLabel product, based on the handler for labels product.
template<typename L1, typename L2, template<typename, typename> class LabelProduct>
class TSLabelProdImpl<  CounterLabel<L1>, CounterLabel<L2>,
                        typename ALabel<L1, L2, LabelProduct>::tsprod_label, LabelProduct>:
        public ILabelProd<  CounterLabel<L1>, CounterLabel<L2>,
                            typename ALabel<L1, L2, LabelProduct>::tsprod_label> {
 public:
    /// a shortcut for the label product
    using P = typename ALabel<L1, L2, LabelProduct>::autprod_label;

    explicit TSLabelProdImpl() {}
    template<typename... Args>
    explicit TSLabelProdImpl(std::size_t counter_offset, std::size_t acceptance_offset, Args... args)
    : _lhandler(LabelProduct<L1, L2>(args...))
    , _counter_offset(counter_offset)
    , _acceptance_offset(acceptance_offset) {}

    ~TSLabelProdImpl() {}

    const CounterLabel<L1> lhs(const CounterLabel<P> &cl) const override {
        // keep the `_counter_offset` first counters
        std::vector<CounterOperationList> left_ops(cl.get_operations().begin(),
                                                   cl.get_operations().begin() + _counter_offset);

        // keep the acceptance conditions below `_acceptance_offset`
        std::set<std::size_t> left_accs(cl.get_acceptance().begin(),
                                        cl.get_acceptance().lower_bound(_acceptance_offset));

        // rebuild a CounterLabel
        return CounterLabel<L1>(_lhandler.lhs(cl.letter()), left_ops, left_accs);
    }

    const CounterLabel<L2> rhs(const CounterLabel<P> &cl) const override {
        // remove the `_counter_offset` first counters
        std::vector<CounterOperationList> right_ops(cl.get_operations().begin() + _counter_offset,
                                                    cl.get_operations().end());
        assert(right_ops.size() + _counter_offset == cl.get_operations().size());

        std::set<std::size_t> right_accs;
        // keep the acceptance conditions above `_acceptance_offset`, and shift them down
        std::transform(cl.get_acceptance().lower_bound(_acceptance_offset),
                       cl.get_acceptance().end(),
                       std::inserter(right_accs, right_accs.end()),
                       [this](std::size_t x) {
                           assert(x >= _acceptance_offset);
                           return x - _acceptance_offset; });

        // rebuild a CounterLabel
        return CounterLabel<L2>(_lhandler.rhs(cl.letter()), right_ops, right_accs);
    }

    const CounterLabel<P>
        build(const CounterLabel<L1> &l, const CounterLabel<L2> &r) const override {
        assert(l.get_operations().size() == _counter_offset);

        // regroup the counter operations
        std::vector<CounterOperationList> counters;
        counters.insert(counters.end(), l.get_operations().begin(), l.get_operations().end());
        counters.insert(counters.end(), r.get_operations().begin(), r.get_operations().end());

        // regroup the acceptance conditions, by shifting up those from `r`
        std::set<std::size_t> accs(l.get_acceptance().begin(), l.get_acceptance().end());
        std::transform(r.get_acceptance().begin(), r.get_acceptance().end(),
                       std::inserter(accs, accs.end()),
                       [this](std::size_t x) { return x + _acceptance_offset; });

        // rebuild a CounterLabel
        return CounterLabel<P>(_lhandler.build(l.letter(), r.letter()), counters, accs);
    }

 private:
    LabelProduct<L1, L2> _lhandler;
    std::size_t _counter_offset;
    std::size_t _acceptance_offset;
};

/// The class CLWrapper.
/// based on the above TSLabelProdImpl, we have to "curry" the template arguments.
template<template<typename, typename> class LabelProd>
struct CLWrapper {
 private:
    /// helper structs to work around the impossibility to specialize template type aliases.
    template<typename L1, typename L2> struct _type {};
    template<typename L1, typename L2> struct _type<CounterLabel<L1>, CounterLabel<L2>> {
        using type = TSLabelProdImpl<CounterLabel<L1>, CounterLabel<L2>, typename ALabel<L1, L2, LabelProd>::tsprod_label, LabelProd>;
    };

 public:
    /// see requirement (3) at the beginning of the file
    template<typename L1, typename L2> using type = typename _type<L1, L2>::type;
};

/// Helper struct to enforce (6)
/// Once again, workaround the impossibility to specialize templated typedef
template <typename Q, typename S1, typename S2, typename S, template<typename A, typename B> class LP>
struct _TSP {};

template<   typename Q1, typename L1, typename Q2, typename L2,
            template<typename, typename> class AutLP>
struct _TSP<StateProd<Q1, Q2>, CounterLabel<L1>, CounterLabel<L2>, typename ALabel<L1, L2, AutLP>::tsprod_label, AutLP> {
    using type = TransitionSystemProduct<   Q1, CounterLabel<L1>,
                                            Q2, CounterLabel<L2>,
                                            CLWrapper<AutLP>::template type>;
};

/// Final helper to enforce (6), by currying template arguments of _TSP
template<typename L1, typename L2, template<typename, typename> class LP>
struct TSP {
    template<typename Q, typename S> using type = typename _TSP<Q, L1, L2, S, LP>::type;
};

/// Useful alias for the base class of CounterAutomatonProduct
/// @todo intermediate aliases would make this one more readable
template<   typename Q1, typename S1, template<typename Q1_, typename S1_> class TS1,
            typename Q2, typename S2, template<typename Q2_, typename S2_> class TS2,
            template<typename, typename> class LabelProduct>
using CAPBase = CounterAutomaton<StateProd<Q1, Q2>,
            typename ALabel<S1, S2, LabelProduct>::autprod_label,
            TSP<CounterLabel<S1>, CounterLabel<S2>, LabelProduct>::template type>;

/// The class CounterAutomatonProduct
/// Has a lot of arguments in the general case
template<   typename Q1, typename S1, template<typename Q1_, typename S1_> class TS1,
            typename Q2, typename S2, template<typename Q2_, typename S2_> class TS2,
            template<typename S1_, typename S2_> class LabelProduct>
class CounterAutomatonProduct: public CAPBase<Q1, S1, TS1, Q2, S2, TS2, LabelProduct> {
    /// A useful typedef for the base type.
    using super_type = CAPBase<Q1, S1, TS1, Q2, S2, TS2, LabelProduct>;

    /// typedef for the TS label types
    using TSLabelType = typename CLWrapper<LabelProduct>::template type<CounterLabel<S1>, CounterLabel<S2>>;

 public:
    /// Constructor from two other CounterAutomata
    template<typename... Args>
    explicit CounterAutomatonProduct(CounterAutomaton<Q1, S1, TS1> &lhs,
                                     CounterAutomaton<Q2, S2, TS2> &rhs,
                                     Args... args)
    : super_type(lhs.num_counters() + rhs.num_counters(),
                 lhs.num_acceptance_sets() + rhs.num_acceptance_sets()) {
        super_type::_transition_system =
            new typename super_type::transition_system_t(
                lhs.transition_system(),
                rhs.transition_system(),
                TSLabelType(lhs.num_counters(), lhs.num_acceptance_sets(), args...));
        this->set_initial_state(StateProd<Q1, Q2>(*lhs.initial_state(), *rhs.initial_state()));
    }

    /// @todo no other methods to overload?
};

}  // namespace automata
}  // namespace spaction

/// We now define our LabelProduct for the labels FormulaList
/// Work around the impossibility to specialize templated typedef

#include "ConstantExpression.h"

namespace spaction {
namespace automata {

template<typename A, typename B>
class _AutLabelProduct {};

/// A specialization for the CounterAutomata produced by CltlTranslator.
template<>
class _AutLabelProduct<CltlTranslator::FormulaList, CltlTranslator::FormulaList>:
        public IAutLabelProd<   CltlTranslator::FormulaList,
                                CltlTranslator::FormulaList,
                                CltlTranslator::FormulaList> {
 public:
    /// typdef for the base class
    using Base = IAutLabelProd< CltlTranslator::FormulaList,
                                CltlTranslator::FormulaList,
                                CltlTranslator::FormulaList>;

    virtual product_type build(const lhs_type &l, const rhs_type &r) const override {
        if (l.empty() and r.empty())
            return {};
                                    
        product_type res;
        /// Check that l and r are sets
        auto compare = CltlTranslator::get_formula_order();
        assert(std::is_sorted(l.begin(), l.end(), compare));
        assert(std::adjacent_find(l.begin(), l.end()) == l.end());
        assert(std::is_sorted(r.begin(), r.end(), compare));
        assert(std::adjacent_find(r.begin(), r.end()) == r.end());

        std::set_union(l.begin(), l.end(), r.begin(), r.end(), std::back_inserter(res), compare);
        /// Check that the merge yields a union
        assert(std::is_sorted(res.begin(), res.end(), compare));
        assert(std::adjacent_find(res.begin(), res.end()) == res.end());


        for (auto &f : res) {
            if (f->formula_type() == CltlFormula::kConstantExpression) {
                spaction::ConstantExpression *ce = static_cast<spaction::ConstantExpression *>(f.get());
                if (!ce->value()) {
                    res = { f };
                    return res;
                }
            }
        }

        product_type negres;
        std::transform(res.begin(), res.end(), std::back_inserter(negres), [](const CltlFormulaPtr &f){ return f->creator()->make_not(f); });
        std::sort(negres.begin(), negres.end(), compare);
        product_type tmp;
        std::set_intersection(res.begin(), res.end(), negres.begin(), negres.end(), std::back_inserter(tmp), compare);
        if (! tmp.empty()) {
            CltlFormulaFactory *factory = l.empty() ? (*r.begin())->creator() : (*l.begin())->creator();
            res = { factory->make_constant(false) };
        }
        return res;
    }

    /// @todo   these definitions of lhs and rhs may not yield the expected results, but it's rather
    ///         a problem with the current implementation of UndeterministicTransitionSystem.
    ///         When asking the succs with label L, we actually want to iterate over all the
    ///         transition with label compatible with L, rather than L exactly.
    virtual lhs_type lhs(const product_type &p) const override {
        return p;
    }
    virtual rhs_type rhs(const product_type &p) const override {
        return p;
    }
};

}  // namespace automata
}  // namespace spaction

#include <spot/ltlast/multop.hh>
#include <spot/tgba/formula2bdd.hh>
#include "cltl2spot.h"

namespace spaction {
namespace automata {

/// A specialization to combine the CounterAutomata produced by CltlTranslator, and the TGBA from
/// SPOT, through the adapter tgba_ca.
template<>
class _AutLabelProduct<CltlTranslator::FormulaList, bdd> :
    public IAutLabelProd<   CltlTranslator::FormulaList,
                            bdd,
                            CltlTranslator::FormulaList> {
 public:
    /// typdef for the base class
    using Base = IAutLabelProd< CltlTranslator::FormulaList,
                                bdd,
                                CltlTranslator::FormulaList>;

    explicit _AutLabelProduct(): _AutLabelProduct(nullptr, nullptr) {}
    explicit _AutLabelProduct(spot::bdd_dict *d, CltlFormulaFactory *f): _dict(d), _factory(f) {}

    ~_AutLabelProduct() {
        if (_dict != nullptr)
            _dict->unregister_all_my_variables(this);
    }

    virtual product_type build(const lhs_type &l, const rhs_type &r) const override {
        // translate the bdd to a FormulaList
        lhs_type rr;

        // translate the bdd to a spot LTL formula
        const spot::ltl::formula *fspot = spot::bdd_to_formula(r, _dict);
        // convert the spot formula to a spaction formula
        CltlFormulaPtr fspaction = spot2cltl(fspot, _factory);

        // "unfold" the big and formula to a list of conjuncts
        std::stack<CltlFormulaPtr> todo;
        todo.push(fspaction);
        while (!todo.empty()) {
            CltlFormulaPtr f = todo.top();
            todo.pop();

            if (f->formula_type() == CltlFormula::kBinaryOperator) {
                const BinaryOperator *bf = static_cast<const BinaryOperator *>(f.get());
                assert(bf);
                assert(bf->operator_type() == BinaryOperator::kAnd);
                todo.push(bf->left());
                todo.push(bf->right());
                continue;
            }
            
            if (f->formula_type() == CltlFormula::kAtomicProposition) {
                rr.push_back(f);
                continue;
            }

            if (f->formula_type() == CltlFormula::kUnaryOperator) {
                const UnaryOperator *uf = static_cast<const UnaryOperator *>(f.get());
                assert(uf);
                assert(uf->operator_type() == UnaryOperator::kNot);
                rr.push_back(f);
                continue;
            }
            
            assert(false);
        }

        // sort the produced list
        auto compare = CltlTranslator::get_formula_order();
        std::sort(rr.begin(), rr.end(), compare);

        // call the version to combine two FormulaList
        return _AutLabelProduct<lhs_type, lhs_type>().build(l, rr);
    }

    /// @todo   these definitions of lhs and rhs may not yield the expected results, but it's rather
    ///         a problem with the current implementation of UndeterministicTransitionSystem.
    ///         When asking the succs with label L, we actually want to iterate over all the
    ///         transition with label compatible with L, rather than L exactly.
    virtual lhs_type lhs(const product_type &p) const override {
        return p;
    }
    virtual rhs_type rhs(const product_type &p) const override {
        spot::ltl::multop::vec *vector = new spot::ltl::multop::vec();
        for (auto &f : p) {
            vector->push_back(cltl2spot(f));
        }
        const spot::ltl::formula *fspot = spot::ltl::multop::instance(spot::ltl::multop::And, vector);
        return spot::formula_to_bdd(fspot, _dict, (void*)this);
    }

 private:
    spot::bdd_dict *_dict;
    CltlFormulaFactory *_factory;
};

template<typename A, typename B> using AutLabelProduct = _AutLabelProduct<A, B>;

/// A factory function
template<   typename Q1, typename S1, template<typename Q1_, typename S1_> class TS1,
            typename Q2, typename S2, template<typename Q2_, typename S2_> class TS2,
            typename... Args>
CounterAutomatonProduct<Q1, S1, TS1, Q2, S2, TS2, AutLabelProduct>
make_aut_product(CounterAutomaton<Q1, S1, TS1> &l, CounterAutomaton<Q2, S2, TS2> &r, Args... args) {
    return CounterAutomatonProduct<Q1, S1, TS1, Q2, S2, TS2, AutLabelProduct>(l, r, args...);
}

}  // namespace automata
}  // namespace spaction

#endif  // SPACTION_INCLUDE_AUTOMATA_COUNTERAUTOMATONPRODUCT_H_
