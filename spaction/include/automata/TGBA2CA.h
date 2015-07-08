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

#include "bdd_util.h"
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

/// The transition iterator
template<typename Q, typename S>
class TGBATSIterator : public ITransitionBaseIterator<Q,S,TGBATSIterator<Q,S>> {
    using super_type = ITransitionBaseIterator<Q,S,TGBATSIterator<Q,S>>;
 public:
    explicit TGBATSIterator(): TGBATSIterator(nullptr, nullptr, nullptr) {}
    /// the iterator does not acquire the source state, but does acquire the iterator
    explicit TGBATSIterator(spot::state *s, spot::twa_succ_iterator *t, TGBATransitionSystem<Q,S> *ts)
    : _source(s)
    , _it(t)
    , _ts(ts)
    , _n(0)
    { if (_it) _it->first(); }

    virtual ~TGBATSIterator() {
        if (_ts)
            _ts->_tgba->release_iter(_it);
    }

    explicit TGBATSIterator(const TGBATSIterator &other)
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
    TGBATSIterator& operator=(const TGBATSIterator &other) {
        if (this != &other) {
            _source = other._source;
            if (_ts)
                _ts->_tgba->release_iter(_it);
            _ts = other._ts;
            _n = other._n;
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
        return *this;
    }
    explicit TGBATSIterator(TGBATSIterator &&) = delete;
    TGBATSIterator& operator=(TGBATSIterator &&) = delete;

    bool is_equal(const super_type & rhs) const override {
        /// the spot::tgba_succ_iterator do not work that way and do not have a comparison operator
        /// instead they have a 'done' method, which is the intent of our operator!=
        /// we thus have to assume that `rhs` MUST be the end iterator
        assert(static_cast<const TGBATSIterator &>(rhs)._it == nullptr);
        return _it == nullptr || _it->done();
    }

    TGBATSIterator *clone() const override {
        return new TGBATSIterator(*this);
    }

    TransitionPtr<Q, S> operator*() override {
        return TransitionPtr<Q, S>(_ts->_make_transition(_source, _it->current_state(), get_label()), _ts->get_control_block());
    }

    S get_label() const override {
        return CounterLabel<bdd>(_it->current_condition(), {}, _it->current_acceptance_conditions());
    }
    const Q get_source() const override { return _source; }
    const Q get_sink() const override { return _it->current_state(); }
    template<bool U = is_counter_label<S>::value>
    typename std::enable_if<U, accs_t>::type _get_acceptance() const {
        return _it->current_acceptance_conditions();
    }
    template<bool U = is_counter_label<S>::value>
    typename std::enable_if<U, typename LetterType<S>::type>::type _get_letter() const {
        return _it->current_condition();
    }
    template<bool U = is_counter_label<S>::value>
    inline typename std::enable_if<U, std::vector<CounterOperationList>>::type _get_operations() const {
        return {};
    }

    const super_type& operator++() override {
        assert(_it != nullptr);
        _it->next();
        ++_n;
        assert(_n != 0); // to detect overflows
        return *this;
    }

 private:
    spot::state *_source;
    spot::twa_succ_iterator *_it;
    TGBATransitionSystem<Q,S> *_ts;
    unsigned _n;
};

template<>
class TGBATransitionSystem<spot::state*, CounterLabel<bdd>>:
    public TransitionSystem<spot::state*, CounterLabel<bdd>, TGBATransitionSystem<spot::state*, CounterLabel<bdd>>, TGBATSIterator<spot::state*, CounterLabel<bdd>>> {
    /// useful typedefs
    using super_type = TransitionSystem<spot::state*, CounterLabel<bdd>, TGBATransitionSystem<spot::state*, CounterLabel<bdd>>, TGBATSIterator<spot::state*, CounterLabel<bdd>>>;
    using Q = spot::state*;
    using S = CounterLabel<bdd>;
 public:
    using TransitionBaseIterator = TGBATSIterator<spot::state*, CounterLabel<bdd>>;
    friend class TGBATSIterator<spot::state*, CounterLabel<bdd>>;
    using TransitionIterator = typename super_type::TransitionIterator;
    /// constructor
    explicit TGBATransitionSystem(spot::const_twa_ptr t)
    : super_type(new RefControlBlock<Transition<Q, S>>(std::bind(&TGBATransitionSystem::_delete_transition, this, std::placeholders::_1)),
                 std::make_shared<DataBddDict>(t->get_dict()))
    , _tgba(t)
    {
        assert(_tgba);
        assert(_tgba->acc().is_generalized_buchi());
        // ensure that we can use the same acceptance conditions as _tgba
        assert(_tgba->acc().num_sets() == _tgba->acc().get_acceptance().used_sets().max_set());
    }

    /// destructor
    ~TGBATransitionSystem() {}

    const spot::bdd_dict_ptr tgba_dict() const { return _tgba->get_dict(); }

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
        for (auto a : s.get_acceptance().sets()) {
            os << "Acc(" << a << ")" << std::endl;
        }
    }

private:
    /// the underlying tgba
    spot::const_twa_ptr _tgba;

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
    explicit tgba_ca(spot::const_twa_ptr t): CounterAutomaton(0, t->acc().num_sets(), t) {
        this->set_initial_state(t->get_init_state());
        this->get_dict()->register_all_propositions_of(t, this);
    }

    /// destructor
    ~tgba_ca() {
        this->get_dict()->unregister_all_my_variables(this);
    }

    spot::bdd_dict_ptr get_dict() const {
        transition_system_t * tmp = static_cast<transition_system_t*>(_transition_system);
        assert(tmp);
        return tmp->tgba_dict();
    }

};

}  // namespace automata
}  // namespace spaction

#endif  // SPACTION_INCLUDE_AUTOMATA_TGBA2CA_H_
