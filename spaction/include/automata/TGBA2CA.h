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

#ifndef SPACTION_INCLUDE_AUTOMATA_TGBA2CA_H_
#define SPACTION_INCLUDE_AUTOMATA_TGBA2CA_H_

#include <spot/twa/twa.hh>
#include <spot/twa/bddprint.hh>
#include <spot/twaalgos/reachiter.hh>

#include "automata/CounterAutomaton.h"

namespace std {

template<> struct less<spot::state *> {
    bool operator()(spot::state *a, spot::state *b) const {
        return a->compare(b) < 0;
    }
};

template<typename A, typename B> struct less<std::pair<A,B>> {
    bool operator()(const std::pair<A,B> &lhs, const std::pair<A,B> &rhs) const {
        bool tmp = less<A>()(lhs.first, rhs.first);
        if (!tmp && !less<A>()(rhs.first, lhs.first)) {
            return less<B>()(lhs.second, rhs.second);
        }
        return tmp;
    }
};

}

namespace spaction {
namespace automata {

template<typename Q, typename S>
class TGBATransitionSystem {};

template<>
class TGBATransitionSystem<spot::state*, CounterLabel<bdd>> : public TransitionSystem<spot::state*, CounterLabel<bdd>> {
    /// useful typedefs
    using super_type = TransitionSystem<spot::state*, CounterLabel<bdd>>;
    using Q = spot::state*;
    using S = CounterLabel<bdd>;
public:
    /// constructor
    explicit TGBATransitionSystem(): TGBATransitionSystem(nullptr) {}
    explicit TGBATransitionSystem(spot::const_twa_ptr t)
    : super_type(new RefControlBlock<Transition<Q, S>>(std::bind(&TGBATransitionSystem::_delete_transition, this, std::placeholders::_1)))
    , _tgba(t)
    {
        if (_tgba) {
            // create the acceptance conditions
            auto accs = _and_operands(_tgba->acc());
            std::size_t i = 0;
            for (auto a : accs) {
                if (_accs_map.find(a) == _accs_map.end()) {
                    _accs_map[a] = i++;
                }
            }
        }
    }

    /// destructor
    ~TGBATransitionSystem() {}

    const spot::bdd_dict_ptr tgba_dict() const { return _tgba->get_dict(); }
    std::size_t get_acceptance(unsigned f) const {
        auto it = _accs_map.find(f);
        assert(it != _accs_map.end());
        return it->second;
    }

    /// deliberately left unimplemented
    void add_state(const Q &state) override {}
    /// deliberately left unimplemented
    void remove_state(const Q &state) override {}
    /// deliberately always succeeds, testing the state would require to walk the underlying tgba
    bool has_state(const Q &state) const override { return true; }

    /// @todo serves as a public accessor to _make_transition?
    const Transition<Q, S> *add_transition(const Q &source, const Q &sink, const S &label) override {
        assert(false);
        return nullptr;
    }
    /// nothing to do, since transitions are not stored
    void remove_transition(const Q &source, const Q &sink, const S &label) override {}

    void print_state(std::ostream &os, const Q &q) const override {
        os << _tgba->format_state(q);
    }
    void print_label(std::ostream &os, const S &s) const override {
        os << "{" << bdd_format_formula(_tgba->get_dict(), s.letter()) << "}" <<  ":[";
        for (std::size_t i = 0; i < s.num_counters(); ++i) {
            const CounterOperationList &counter = s.counter_operations(i);
            os << "(";
            for (auto c : counter) {
                os << print_counter_operation(c) << ",";
            }
            os << "),";
        }
        os << "]" << std::endl;
        // print acceptance conditions
        for (auto a : s.get_acceptance()) {
            os << "Acc(" << a << ")" << std::endl;
        }
    }

private:
    /// the underlying tgba
    spot::const_twa_ptr _tgba;
    /// helper map for acceptance conditions
    std::map<unsigned, std::size_t> _accs_map;

    static std::set<unsigned> _and_operands(const spot::acc_cond::mark_t &conds) {
        std::set<unsigned> result;
        std::vector<unsigned> tmp = conds.sets();
        result.insert(tmp.begin(), tmp.end());
        return result;
    }

    static std::set<unsigned> _and_operands(const spot::acc_cond &conds) {
        if (! conds.is_generalized_buchi())
            throw std::runtime_error("acceptance other than generalized buchi are not supported");

        return _and_operands(conds.all_sets());
    }

    /// The transition iterator
    class TransitionBaseIterator : public super_type::TransitionBaseIterator {
     public:
        explicit TransitionBaseIterator(): TransitionBaseIterator(nullptr, nullptr, nullptr) {}
        /// the iterator does not acquire the source state, but does acquire the iterator
        explicit TransitionBaseIterator(spot::state *s, spot::twa_succ_iterator *t, TGBATransitionSystem *ts)
        : _source(s)
        , _it(t)
        , _ts(ts)
        , _n(0)
        { if (_it) _it->first(); }

        virtual ~TransitionBaseIterator() {
            if (_ts)
                _ts->_tgba->release_iter(_it);
        }

        explicit TransitionBaseIterator(const TransitionBaseIterator &other)
        : _source(other._source)
        , _ts(other._ts)
        , _n(other._n)
        {
            if (other._it == nullptr) {
                _it = nullptr;
            } else {
                assert(_ts != nullptr);
                _it = other._ts->_tgba->succ_iter(_source);
                _it->first();
                for (unsigned i = 0 ; !_it->done() && i != _n ; ++i) {
                    _it->next();
                }
            }
        }
        TransitionBaseIterator& operator=(const TransitionBaseIterator &) = delete;
        explicit TransitionBaseIterator(TransitionBaseIterator &&) = delete;
        TransitionBaseIterator& operator=(TransitionBaseIterator &&) = delete;

        bool is_equal(const super_type::TransitionBaseIterator& rhs) const override {
            const TransitionBaseIterator &r = static_cast<const TransitionBaseIterator &>(rhs);
            /// the spot::tgba_succ_iterator do not work that way and do not have a comparison operator
            /// instead they have a 'done' method, which is the intent of our operator!=
            /// we thus have to assume that `rhs` MUST be the end iterator
            assert(r._it == nullptr);
            return _it == nullptr || _it->done();
        }

        TransitionBaseIterator *clone() const override {
            return new TransitionBaseIterator(*this);
        }

        TransitionPtr<Q, S> operator*() override {
            std::vector<CounterOperationList> op_list;
            auto tmp = _ts->_and_operands(_it->current_acceptance_conditions());
            std::set<std::size_t> accs;
            std::transform(tmp.begin(), tmp.end(), std::inserter(accs, accs.end()),
                           [this](unsigned f) { return this->_ts->get_acceptance(f); });
            CounterLabel<bdd> cl(_it->current_condition(), op_list, accs);
            return TransitionPtr<Q, S>(_ts->_make_transition(_source, _it->current_state(), cl), _ts->get_control_block());
        }

        const TransitionBaseIterator& operator++() override {
            assert(_it != nullptr);
            _it->next();
            ++_n;
            assert(_n != 0); // to detect overflows
            return *this;
        }

     private:
        spot::state *_source;
        spot::twa_succ_iterator *_it;
        TGBATransitionSystem *_ts;
        unsigned _n;
    };

    /// this implementation relies on spot DFS and is quite inefficient for our purpose, but it works
    class StateBaseIterator : public super_type::StateBaseIterator, public spot::tgba_reachable_iterator_depth_first {
     public:
        explicit StateBaseIterator(spot::const_twa_ptr t, bool is_end=false): spot::tgba_reachable_iterator_depth_first(t) {
            start(); run(); end();
            if (is_end)
                _n = seen.size()+1;
            else
                _n = 1;
        }

        explicit StateBaseIterator(const StateBaseIterator &other): spot::tgba_reachable_iterator_depth_first(other.aut_), _n(other._n) {
            start(); run(); end();
        }

        StateBaseIterator &operator=(const StateBaseIterator &) = delete;

        ~StateBaseIterator() {}

        bool is_equal(const super_type::StateBaseIterator &rhs) const {
            const StateBaseIterator *other = static_cast<const StateBaseIterator *>(&rhs);
            assert(other->aut_ == aut_);
            return _n == other->_n;
        }

        StateBaseIterator *clone() const override {
            return new StateBaseIterator(*this);
        }

        Q operator*() override {
            assert(_n <= seen.size());
            for (auto &p : seen) {
                if (p.second == _n) {
                    return p.first->clone();
                }
            }
            assert(false);
            return seen.begin()->first->clone();
        }

        const StateBaseIterator& operator++() override {
            assert(_n <= seen.size());
            ++_n;
            return *this;
        }

     private:
        int _n;
    };

    TransitionBaseIterator *_successor_begin(const Q &state, const S *label) override {
        return new TransitionBaseIterator(state, _tgba->succ_iter(state), this);
    }
    TransitionBaseIterator *_successor_end(const Q &state) override {
        return new TransitionBaseIterator();
    }

    /// @note not implemented yet
    TransitionBaseIterator *_predecessor_begin(const Q &state, const S *label) override {
        assert(false);
        return nullptr;
    }
    /// @note not implemented yet
    TransitionBaseIterator *_predecessor_end(const Q &state) override {
        assert(false);
        return nullptr;
    }

    StateBaseIterator *_state_begin() override { return new StateBaseIterator(_tgba); }
    StateBaseIterator *_state_end() override { return new StateBaseIterator(_tgba, true); }
};

class tgba_ca : public CounterAutomaton<spot::state*, bdd, TGBATransitionSystem> {
    using Q = spot::state *;
    using S = bdd;
public:
    /// constructor
    explicit tgba_ca(spot::const_twa_ptr t): CounterAutomaton(0, t->acc().num_sets()) {
        _transition_system = new transition_system_t(t);
        this->set_initial_state(t->get_init_state());
    }

    /// destructor
    ~tgba_ca() {}

};

}  // namespace automata
}  // namespace spaction

#endif  // SPACTION_INCLUDE_AUTOMATA_TGBA2CA_H_
