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

/// CAProd<Q1,L1,Q2,L2>             derives from        CA<pair<Q1,Q2>, pair<L1,L2>,TSProd>
/// TSProd<Q1,S1,Q2,S2,LabelProd>   should derive from  TS<pair<Q1,Q2>, CounterLabel<pair<L1,L2>>>
///
///     The CAProd is built from
/// CA<Q1,L1>                       and CA<Q2,L2>
///     The underlying TSProd is thus built from
/// TS<Q1,CounterLabel<L1>>         and TS<Q2,CounterLabel<T2>>
///
///     We thus want a match between
/// S1 = CounterLabel<L1>           and S2 = CounterLabel<L2>
/// LabelProd<S1,S2>::type = CounterLabel<pair<L1,L2>
/// TSProd<Q1,S1,Q2,S2,LabelProd> <<= TS<pair<Q1,Q2>, CounterLabel<pair<L1,L2>>>

/// A trivial implementation of ILabelProd, serves as a general template for further specialization.
template<typename A, typename B, typename C>
class LabelProdImpl : public ILabelProd<A, B, C> {
    const A lhs(const C &) const override;
    const B rhs(const C &) const override;
    const C build(const A &, const B &) const override;
};

/// Partial specialization of ILabelProd to use with CounterLabel
template<typename A, typename B>
class LabelProdImpl<CounterLabel<A>, CounterLabel<B>, CounterLabel<std::pair<A, B>>> :
    public ILabelProd<CounterLabel<A>, CounterLabel<B>, CounterLabel<std::pair<A, B>>> {
 public:
    /// default constructor
    explicit LabelProdImpl(): LabelProdImpl(0, 0) {}
    explicit LabelProdImpl(std::size_t counter_offset, std::size_t acceptance_offset)
    : _counter_offset(counter_offset)
    , _acceptance_offset(acceptance_offset) {}

    virtual ~LabelProdImpl() {}

    const CounterLabel<A> lhs(const CounterLabel<std::pair<A, B>> &cl) const override {
        // keep the `_counter_offset` first counters
        std::vector<CounterOperationList> left_ops(cl.get_operations().begin(),
                                                   cl.get_operations().begin() + _counter_offset);

        // keep the acceptance conditions below `_acceptance_offset`
        std::set<std::size_t> left_accs(cl.get_acceptance().begin(),
                                        cl.get_acceptance().lower_bound(_acceptance_offset));

        // rebuild a CounterLabel
        return CounterLabel<A>(cl.letter().first, left_ops, left_accs);
    }

    const CounterLabel<B> rhs(const CounterLabel<std::pair<A, B>> &cl) const override {
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
        return CounterLabel<B>(cl.letter().second, right_ops, right_accs);
    }

    const CounterLabel<std::pair<A, B>>
    build(const CounterLabel<A> &l, const CounterLabel<B> &r) const override {
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

        // @todo std::make_pair moves its arguments, is it invalid resource stealing?
        return CounterLabel<std::pair<A, B>>(std::make_pair(l.letter(), r.letter()),
                                             counters, accs);
    }

 private:
    const std::size_t _counter_offset;
    const std::size_t _acceptance_offset;
};

/// Workaround the impossibility to specialize templated typedef
template<typename A, typename B>
struct _LabelProd {};

template<typename A, typename B>
struct _LabelProd<CounterLabel<A>, CounterLabel<B>> {
    typedef LabelProdImpl<CounterLabel<A>, CounterLabel<B>, CounterLabel<std::pair<A, B>>> type;
};

/// Type wrapper: takes two template arguments (as in the interface of TransitionSystemProduct)
/// and returns the correct specialized implementation of ILabelProd
template<typename A, typename B> using LabelProd = typename _LabelProd<A, B>::type;

/// Once again, workaround the impossibility to specialize templated typedef
template <typename Q, typename S>
struct _TSProd {};

template<typename Q1, typename L1, typename Q2, typename L2>
struct _TSProd<StateProd<Q1, Q2>, CounterLabel<std::pair<L1, L2>>> {
    typedef TransitionSystemProduct<Q1, CounterLabel<L1>,
                                    Q2, CounterLabel<L2>,
                                    LabelProd> type;
};

/// Type wrapper: takes two template arguments (as in the interface of CounterAutomaton)
/// and returns the correct instantiation of TransitionSystemProduct
template<typename Q, typename S> using TSProd = typename _TSProd<Q, S>::type;

/// The class CounterAutomatonProduct
/// Has a lot of arguments in the general case
template<   typename Q1, typename S1, template<typename Q1_, typename S1_> class TS1,
            typename Q2, typename S2, template<typename Q2_, typename S2_> class TS2>
class CounterAutomatonProduct:
    public CounterAutomaton<StateProd<Q1, Q2>, std::pair<S1, S2>, TSProd> {
 public:
    /// A useful typedef for the base type.
    typedef CounterAutomaton<StateProd<Q1, Q2>, std::pair<S1, S2>, TSProd> super_type;

    /// Constructor from two other CounterAutomata
    explicit CounterAutomatonProduct(CounterAutomaton<Q1, S1, TS1> &lhs,
                                     CounterAutomaton<Q2, S2, TS2> &rhs)
    : super_type(lhs.num_counters() + rhs.num_counters(),
                 lhs.num_acceptance_sets() + rhs.num_acceptance_sets()) {
        super_type::_transition_system =
            new typename super_type::transition_system_t(
                lhs.transition_system(),
                rhs.transition_system(),
                LabelProd<CounterLabel<S1>, CounterLabel<S2>>(lhs.num_counters(),
                                                              lhs.num_acceptance_sets()));
        this->set_initial_state(StateProd<Q1, Q2>(*lhs.initial_state(), *rhs.initial_state()));
    }

    /// @todo no other methods to overload?
};

/// A factory function (maybe not very useful)
template<   typename Q1, typename S1, template<typename Q1_, typename S1_> class TS1,
            typename Q2, typename S2, template<typename Q2_, typename S2_> class TS2>
CounterAutomatonProduct<Q1, S1, TS1, Q2, S2, TS2>
make_aut_product(CounterAutomaton<Q1, S1, TS1> &l, CounterAutomaton<Q2, S2, TS2> &r) {
    return CounterAutomatonProduct<Q1, S1, TS1, Q2, S2, TS2>(l, r);
}

}  // namespace automata
}  // namespace spaction

#endif  // SPACTION_INCLUDE_AUTOMATA_COUNTERAUTOMATONPRODUCT_H_
