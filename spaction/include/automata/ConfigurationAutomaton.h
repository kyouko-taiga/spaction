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

#ifndef SPACTION_INCLUDE_AUTOMATA_CONFIGURATIONAUTOMATON_H_
#define SPACTION_INCLUDE_AUTOMATA_CONFIGURATIONAUTOMATON_H_

namespace spaction {
namespace automata {

/// @note   mycompare(x,y) returns a value:
///             < 0 iff x < y
///             > 0 iff x > y
///             = 0 iff x == y
/// @todo move in another namespace/file?
template<typename Q>
struct mycompare {
    int operator()(const Q &lhs, const Q &rhs) const {
        if (std::equal_to<Q>(lhs, rhs))
            return 0;
        if (std::less<Q>()(lhs, rhs))
            return -1;
        return 1;
    }
};

/// specialization for spot::state*
template<>
struct mycompare<spot::state*> {
    int operator()(spot::state *lhs, spot::state *rhs) const {
        assert(lhs);
        return lhs->compare(rhs);
    }
};

/// specialization
// @todo merge the two pair specialization?
template<typename A>
struct mycompare<std::pair<CltlTranslator::Node *, A>> {
    using pair_type = std::pair<CltlTranslator::Node *, A>;
    int operator()(const pair_type &lhs, const pair_type &rhs) const {
        if (lhs.first->terms() == rhs.first->terms()) {
            return mycompare<A>()(lhs.second, rhs.second);
        } else {
            if (lhs.first->terms() < rhs.first->terms())
                return -1;
            else
                return 1;
        }
    }
};

/// specialization
template<typename A, typename B>
struct mycompare<std::pair<A,B>> {
    using pair_type = std::pair<A,B>;
    int operator()(const pair_type &lhs, const pair_type &rhs) const {
        int first_comp = mycompare<A>()(lhs.first, rhs.first);
        if (first_comp == 0) {
            return mycompare<B>()(lhs.second, rhs.second);
        }
        return first_comp;
    }
};

/// a simple struct to handle a potentially infinite value
/// @todo make it a union
typedef struct {
    bool infinite;
    unsigned int value;
} value_t;

/// a class to represent a configuration of a CA
/// i.e. a tuple <s,v,c> where
///     * s is a state of the automaton
///     * v is the current value of the run (decreasing along a run)
///     * c is a vector indicating the current value of each counter
///
/// Comp is a comparator operator for states.
/// Comp()(x,y) returns a value:
///     < 0 iff x < y
///     > 0 iff x > y
///     = 0 iff x == y
template<typename Q, typename Comp=mycompare<Q>>
class MinMaxConfiguration {
 public:
    explicit MinMaxConfiguration(const Q &q, std::size_t nb_counters)
    : MinMaxConfiguration(q, false, 0, std::vector<unsigned int>(nb_counters, 0))
    {
        assert(nb_counters > 0);
    }

    explicit MinMaxConfiguration(const Q &q, bool is_bounded, unsigned int value, const std::vector<unsigned int> &values)
    : _state(q)
    , _value({!is_bounded, value})
    , _counter_values(values)
    {
        assert(_counter_values.size() > 0);
    }

    ~MinMaxConfiguration() {}

    /// getters
    const Q &state() const { return _state; }
    bool is_bounded() const { return !_value.infinite; }
    unsigned int current_value() const { return _value.value; }
    const std::vector<unsigned int> &values() const { return _counter_values; }

    /// usual comparison operators
    //@todo make them external operators?
    bool operator==(const MinMaxConfiguration &other) const {
        return  Comp()(_state, other._state) == 0
            and _value.infinite == other._value.infinite
            and _value.value == other._value.value
            and _counter_values == other._counter_values;
    }

    bool operator<(const MinMaxConfiguration &other) const {
        if (Comp()(_state, other._state) == 0) {
            if (_value.infinite == other._value.infinite) {
                if (_value.value == other._value.value) {
                    return _counter_values < other._counter_values;
                } else {
                    return _value.value < other._value.value;
                }
            } else {
                return _value.infinite < other._value.infinite;
            }
        } else {
            return Comp()(_state, other._state) < 0;
        }
    }

 private:
    // the state of the automaton
    const Q _state;
    // the current value (must keep track of infinity)
    value_t _value;
    // the current values of the counters
    std::vector<unsigned int> _counter_values;
};

/// a class to represent the configuration automaton's TS
template<typename Q, typename S, template<typename, typename> class TS>
class MinMaxConfigTS : public TransitionSystem<MinMaxConfiguration<Q>, S> {
 public:
    // helper typedef for base
    using super_type = TransitionSystem<MinMaxConfiguration<Q>, S>;

    // @todo clean this up (restrict visibility?)
    explicit MinMaxConfigTS(): MinMaxConfigTS(nullptr, 0) {}

    explicit MinMaxConfigTS(TS<Q, S> *ts, std::size_t nb_counters)
    : super_type(new RefControlBlock<Transition<MinMaxConfiguration<Q>, S>>(
            std::bind(&MinMaxConfigTS::_delete_transition, this, std::placeholders::_1)))
    , _transition_system(ts)
    , _nb_counters(nb_counters)
    {}

    /// from s, return (s, \infty,  0 \dots 0)
    /// useful to define the initial configuration from the initial state
    MinMaxConfiguration<Q> default_config(const Q &state) const {
        return MinMaxConfiguration<Q>(state, _nb_counters);
    }

    virtual void add_state(const MinMaxConfiguration<Q> &state) override {
        _transition_system->add_state(state.state());
    }
    virtual void remove_state(const MinMaxConfiguration<Q> &state) override {
        /// @todo   several configurations may refer to the same state q, and it may be dangerous to
        ///         remove q from the underlying TS without any other check.
        ///         A solution would be to implement a refcount.
        ///         Another solution is to assume that add_state somehow implements such a refcount.
        ///         So far, do nothing.
//        _transition_system->remove_state(state.state());
    }
    virtual bool has_state(const MinMaxConfiguration<Q> &state) const override {
        return _transition_system->has_state(state.state());
    }

    virtual const Transition<MinMaxConfiguration<Q>, S> *add_transition(const MinMaxConfiguration<Q> &source, const MinMaxConfiguration<Q> &sink, const S &label) override {
        return super_type::_make_transition(source, sink, label);
    }
    virtual void remove_transition(const MinMaxConfiguration<Q> &source, const MinMaxConfiguration<Q> &sink, const S &label) override {}

    virtual void print_state(std::ostream &os, const MinMaxConfiguration<Q> &q) const override {
        os << "(";
        _transition_system->print_state(os, q.state());
        os << ", |";
        if (q.is_bounded())
            os << q.current_value();
        else
            os << "inf";
        os << "|, [";
        for (auto &v : q.values())
            os << "," << v;
        os << "])";
    }

    virtual void print_label(std::ostream &os, const S &s) const override {
        _transition_system->print_label(os, s);
    }

 private:
    // @todo make it const?
    TS<Q,S> *_transition_system;
    // the number of counters
    std::size_t _nb_counters;

    class TransitionBaseIterator : public super_type::TransitionBaseIterator {
     public:
        explicit TransitionBaseIterator(MinMaxConfigTS *ts, const MinMaxConfiguration<Q> &s, typename TS<Q,S>::TransitionIterator it)
        : _ts(ts)
        , _source(s)
        , _iterator(it)
        {
            // @debug
            assert(_source.values().size() > 0);
        }
        ~TransitionBaseIterator() {}

        // @todo make the overriden methods virtual for further inheritance?
        bool is_equal(const typename super_type::TransitionBaseIterator& rhs) const override {
            const TransitionBaseIterator *rr = static_cast<const TransitionBaseIterator *>(&rhs);
            return _source == rr->_source && !(_iterator != rr->_iterator);
        }

        // @todo is the iterator properly copied?
        typename super_type::TransitionBaseIterator *clone() const override {
            return new TransitionBaseIterator(*this);
        }

        TransitionPtr<MinMaxConfiguration<Q>, S> operator*() override {
            bool is_sink_bounded = _source.is_bounded();
            unsigned int current_value = _source.current_value();
            std::vector<unsigned int> values = _source.values();
            auto ops = _iterator.get_label().get_operations();
            for (std::size_t k = 0; k != ops.size(); ++k) {
                assert(ops[k].size() == 1);
                if (ops[k][0] & kIncrement) {
                    values[k]++;
                }
                if (ops[k][0] & kCheck) {
                    if (!is_sink_bounded) {
                        is_sink_bounded = true;
                        current_value = values[k];
                    } else if (values[k] < current_value) {
                        current_value = values[k];
                    }
                }
                if (ops[k][0] & kReset) {
                    values[k] = 0;
                }
            }
            assert(_source.is_bounded() ? (is_sink_bounded and current_value <= _source.current_value()) : true);
            auto res =_ts->add_transition(_source, MinMaxConfiguration<Q>((*_iterator)->sink(), is_sink_bounded, current_value, values), _iterator.get_label());
            return TransitionPtr<MinMaxConfiguration<Q>, S>(res, _ts->get_control_block());
        }

        S get_label() const override {
            return _iterator.get_label();
        }

        const MinMaxConfiguration<Q> get_source() const override {
            return _source;
        }
        const MinMaxConfiguration<Q> get_sink() const override {
            bool is_sink_bounded = _source.is_bounded();
            unsigned int current_value = _source.current_value();
            std::vector<unsigned int> values = _source.values();
            auto ops = _iterator.get_label().get_operations();
            for (std::size_t k = 0; k != ops.size(); ++k) {
                assert(ops[k].size() == 1);
                if (ops[k][0] & kIncrement) {
                    values[k]++;
                }
                if (ops[k][0] & kCheck) {
                    if (!is_sink_bounded) {
                        is_sink_bounded = true;
                        current_value = values[k];
                    } else if (values[k] < current_value) {
                        current_value = values[k];
                    }
                }
                if (ops[k][0] & kReset) {
                    values[k] = 0;
                }
            }
            assert(_source.is_bounded() ? (is_sink_bounded and current_value <= _source.current_value()) : true);
            return MinMaxConfiguration<Q>(_iterator.get_sink(), is_sink_bounded, current_value, values);
        }

        const typename super_type::TransitionBaseIterator& operator++() override {
            ++_iterator;
            return *this;
        }

     private:
        MinMaxConfigTS *_ts;
        const MinMaxConfiguration<Q> _source;
        typename TS<Q,S>::TransitionIterator _iterator;
    };

    /// @note   makes a DFS of the configuration TS. Quite inefficient, but there seems to be no
    ///         other solution, given the definition of configurations...
    ///         Maybe explore all confs with values <= size of underlying TS?...
    /// @todo   ensure that it is never used?
    /// @todo   iteration is faulty: it should start from the initial state (or configuration), but
    ///         a TS does not have initial state...
    class StateBaseIterator : public super_type::StateBaseIterator {
     public:
        explicit StateBaseIterator(MinMaxConfigTS *ts, bool is_end=false)
        : _ts(ts)
        , _is_end(is_end)
        {
            // the stack of the DFS
            std::deque<stack_item> _todo;

            //@todo this is faulty: proper iteration should start at initial state, but TS do not
            //      have initial states
            auto q0 = *_ts->_transition_system->states().begin();
            auto c0 = _ts->default_config(q0);
            auto tmpb = (*_ts)(c0).successors().begin();
            auto tmpe = (*_ts)(c0).successors().end();
            _todo.push_front({ tmpb, tmpe });
            _seen.insert(c0);
            while (!_todo.empty()) {
                auto &it = _todo.front().current;
                while (it != _todo.front().end and _seen.count((*it)->sink()) > 0) {
                    ++it;
                }
                if (it != _todo.front().end) {
                    auto c = (*it)->sink();
                    auto ins_res = _seen.insert(c);
                    assert(ins_res.second);  // ensures the insertion did take place
                    auto tmpb = (*_ts)(c).successors().begin();
                    auto tmpe = (*_ts)(c).successors().end();
                    _todo.push_front({ tmpb, tmpe });
                    ++it;
                } else {
                    _todo.pop_front();
                }
            }
            if (_is_end) {
                _current = _seen.size();
                _current_it = _seen.end();
            } else {
                _current = 0;
                _current_it = _seen.begin();
            }
        }
        virtual ~StateBaseIterator() { }

        virtual bool is_equal(const typename super_type::StateBaseIterator& rhs) const override {
            const StateBaseIterator *rr = static_cast<const StateBaseIterator *>(&rhs);
            return _ts == rr->_ts && _current == rr->_current;
        }
        virtual typename super_type::StateBaseIterator *clone() const override {
            return new StateBaseIterator(*this);
        }

        virtual MinMaxConfiguration<Q> operator*() override {
            return *_current_it;
        }
        virtual const typename super_type::StateBaseIterator& operator++() override {
            assert(_current < _seen.size());
            ++_current;
            ++_current_it;
            return *this;
        }

     private:
        MinMaxConfigTS *_ts;
        bool _is_end;

        std::unordered_set<MinMaxConfiguration<Q>> _seen;
        std::size_t _current;
        typename std::unordered_set<MinMaxConfiguration<Q>>::iterator _current_it;

        struct stack_item {
            typename MinMaxConfigTS::TransitionIterator current;
            typename MinMaxConfigTS::TransitionIterator end;
        };
    };

    virtual typename super_type::TransitionBaseIterator *
    _successor_begin(const MinMaxConfiguration<Q> &state, const S *label) override {
        // @debug
        assert(state.values().size() > 0);
        if (label)
            return new TransitionBaseIterator(this, state, (*_transition_system)(state.state()).successors(*label).begin());
        else
            return new TransitionBaseIterator(this, state, (*_transition_system)(state.state()).successors().begin());
    }
    virtual typename super_type::TransitionBaseIterator *
    _successor_end(const MinMaxConfiguration<Q> &state) override {
        return new TransitionBaseIterator(this, state, (*_transition_system)(state.state()).successors().end());
    }

    /// @note deliberately left unimplemented
    virtual typename super_type::TransitionBaseIterator *
    _predecessor_begin(const MinMaxConfiguration<Q> &state, const S *label) override {
        assert(false);
        return nullptr;
    }
    /// @note deliberately left unimplemented
    virtual typename super_type::TransitionBaseIterator *
    _predecessor_end(const MinMaxConfiguration<Q> &state) override {
        assert(false);
        return nullptr;
    }

    /// @todo   state iteration is faulty. See class StateBaseIterator.
    virtual typename super_type::StateBaseIterator *_state_begin() override {
        return new StateBaseIterator(this);
    }
    virtual typename super_type::StateBaseIterator *_state_end() override {
        return new StateBaseIterator(this, true);
    }
};

template<typename Q, typename S, template<typename, typename> class TS>
struct _MinMaxConfigTS {};

template<typename Q, typename S, template<typename, typename> class TS>
struct _MinMaxConfigTS<MinMaxConfiguration<Q>, S, TS> {
    using type = MinMaxConfigTS<Q,S,TS>;
};

/// A template typedef to reorder the template arguments
template<template<typename, typename> class TS>
struct MinMaxConfigurationTS {
    template<typename Q, typename S>
    using type = typename _MinMaxConfigTS<Q,S,TS>::type;
};

/// Make a configuration automaton as a counter automaton
/// Use CA2tgba to see it as a tgba
template<typename Q, typename S, template<typename, typename> class TransitionSystemType>
class MinMaxConfigurationAutomaton : public CounterAutomaton<MinMaxConfiguration<Q>, S, MinMaxConfigurationTS<TransitionSystemType>::template type> {
 public:
    // useful typedef for the super type
    using super_type = CounterAutomaton<MinMaxConfiguration<Q>, S, MinMaxConfigurationTS<TransitionSystemType>::template type>;
    /// constructor
    explicit MinMaxConfigurationAutomaton(const CounterAutomaton<Q, S, TransitionSystemType> &ca)
    : super_type(ca.num_counters(), ca.num_acceptance_sets())
    {
        // @todo add a set_transition_system method to CounterAutomaton to make it cleaner?
        MinMaxConfigTS<Q, CounterLabel<S>, TransitionSystemType> * tmp =
            new MinMaxConfigTS<Q, CounterLabel<S>, TransitionSystemType>(ca.transition_system(), ca.num_counters());
        delete super_type::_transition_system;
        super_type::_transition_system = tmp;
        super_type::set_initial_state(tmp->default_config(*ca.initial_state()));
    }
//    ~MinMaxConfigurationAutomaton();

    // @todo what is to be overriden?
};

// factory function
template<typename Q, typename S, template<typename, typename> class TS>
MinMaxConfigurationAutomaton<Q,S,TS>
make_minmax_configuration_automaton(const CounterAutomaton<Q, S, TS> &a) {
    return MinMaxConfigurationAutomaton<Q,S,TS>(a);
}

}  // namespace automata
}  // namespace spaction

namespace std {

template<typename Q>
struct hash<spaction::automata::MinMaxConfiguration<Q>> {
    typedef spaction::automata::MinMaxConfiguration<Q> argument_type;
    typedef std::size_t result_type;

    result_type operator()(const argument_type &c) const {
        hash<std::pair<Q, std::vector<unsigned int>>> h;
        auto v = c.values();
        v.push_back(c.current_value());
        return h(std::make_pair(c.state(), v));
    }
};

}  // namespace std

#endif  // SPACTION_INCLUDE_AUTOMATA_CONFIGURATIONAUTOMATON_H_
