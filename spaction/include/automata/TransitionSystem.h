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

#ifndef SPACTION_INCLUDE_TRANSITIONSYSTEM_H_
#define SPACTION_INCLUDE_TRANSITIONSYSTEM_H_

namespace spaction {
namespace automata {

template<typename Q, typename S> class Transition;

/// Base class for transition systems implementation.
///
/// A transition system is a tuple `<Q,S,T>` where `Q` is a set of states, `S` is an alphabet and
/// `T` is a a subset of `QxSxQ` representing the transitions.
template<typename Q, typename S> class TransitionSystem {
public:
    virtual ~TransitionSystem() { }

    virtual void add_state(const Q &state) = 0;
    virtual void remove_state(const Q &state) = 0;

    virtual bool has_state(const Q &state) const = 0;

    virtual Transition<Q,S> *add_transition(const Q &source, const Q &sink, const S &label) = 0;

    /// Returns a pointer to an outgoing transition of `source` that is labeled by `label`, unless
    /// returns `nullptr`.
    virtual Transition<Q,S> *find_transition(const Q &source, const S &label) const = 0;

    /// Returns a vector of all the outgoing transitions of `source` that are labeled by `label`.
    virtual std::vector<Transition<Q,S>*> find_all_transitions(const Q &source,
                                                               const S &label) const = 0;

protected:
    /// Internal method to build transitions.
    /// @remarks
    ///     Instances of the class Transition shouldn't be constructed outside a TransitionSystem.
    ///     Subclasses may use this method within their implementation of `add_transition` to
    ///     actually create the transitions object.
    virtual Transition<Q,S> *_make_transition(const Q &source, const Q &sink, const S &label) {
        if (!has_state(source) or !has_state(sink))
            return nullptr;
        return new Transition<Q,S>(source, sink, label);
    }
};

/// Class that stores a deterministic transition system.
///
/// A deterministic transition system is a transition system where for each state `q` of `Q` and
/// each symbol `s` of `S`, there is at most one outgoing transition from `q` labeled by `s`.
template<typename Q, typename S> class DeterministicTransitionSystem :
    public TransitionSystem<Q,S> {
public:
    virtual ~DeterministicTransitionSystem() {}

    virtual void add_state(const Q &state) {
        if (_graph.count(state) > 0) return;
        _graph[state];
    }

    virtual void remove_state(const Q &state) {
        if (!has_state(state)) return;

        _graph.erase(state);
        for (auto source : _graph) {
            for (auto t = source.second.begin(); t != source.second.end(); ++t) {
                if ((*t)->sink() == state)
                    t = source.second.erase(t);
            }
        }
    }

    virtual inline bool has_state(const Q &state) const {
        return _graph.count(state) > 0;
    }

    virtual Transition<Q,S> *add_transition(const Q &source, const Q &sink, const S &label) {
        if (!has_state(source) or !has_state(sink))
            return nullptr;
        
        Transition<Q,S> *t = this->_make_transition(source, sink, label);
        _graph[source][label] = t;
        return t;
    }

    virtual Transition<Q,S> *find_transition(const Q &source, const S &label) const {
        return this->_graph.at(source).at(label);
    }

    virtual std::vector<Transition<Q,S>*> find_all_transitions(const Q &source,
                                                               const S &label) const {
        return std::vector<Transition<Q,S>*>({find_transition(source, label)});
    }

protected:
    /// Stores the transition system as a subset of `QxSxT`.
    std::unordered_map<Q, std::unordered_map<S, Transition<Q,S>*>> _graph;
};

/// Class that stores an undeterministic transition system.
///
/// A undeterministic transition system is a transition system where for each state `q` of `Q` and
/// each symbol `s` of `S`, there can be more than one outgoing transition from `q` labeled by `s`.
template<typename Q, typename S> class UndeterministicTransitionSystem :
    public TransitionSystem<Q,S> {
public:
    virtual ~UndeterministicTransitionSystem() {}
    
    virtual void add_state(const Q &state) {
        if (_graph.count(state) > 0) return;
        _graph[state];
    }

    virtual void remove_state(const Q &state) {
        if (!has_state(state)) return;

        _graph.erase(state);
        for (auto source : _graph) {
            for (auto transitions : source.second) {
                for (auto t = transitions.second.begin(); t != transitions.second.end(); ++t) {
                    if ((*t)->sink() == state)
                        t = transitions.second.erase(t);
                }
            }
        }
    }

    virtual inline bool has_state(const Q &state) const {
        return _graph.count(state) > 0;
    }

    virtual Transition<Q,S> *add_transition(const Q &source, const Q &sink,
                                          const S &label) {
        if (!has_state(source) or !has_state(sink))
            return nullptr;
        
        Transition<Q,S> *t = this->_make_transition(source, sink, label);
        _graph[source][label].push_back(t);
        return t;
    }

    /// @warning
    ///     If `source` has several outgoing transitions labeled by `label`, this method will return
    ///     the first it will find.
    virtual Transition<Q,S> *find_transition(const Q &source, const S &label) const {
        return this->_graph.at(source).at(label).at(0);
    }

    virtual std::vector<Transition<Q,S>*> find_all_transitions(const Q &source,
                                                             const S &label) const {
        return this->_graph.at(source).at(label);
    }

protected:
    /// Stores the transition system as a subset of `QxSx<QxSx<T*>>`.
    std::unordered_map<Q, std::unordered_map<S, std::vector<Transition<Q,S>*>>> _graph;
};

/// Class that represents a transition in a TransitionSystem.
template <typename Q, typename S> class Transition {
public:
    const Q &source() const { return _source; }
    const Q &sink()   const { return _sink; }
    const S &label()       const { return _label; }

protected:
    friend class TransitionSystem<Q,S>;

    const Q _source;
    const Q _sink;
    const S _label;

    explicit Transition(const Q &source, const Q &sink, const S &label) :
    _source(source), _sink(sink), _label(label) {
    }

private:
    /// Copy construction is forbidden.
    Transition(const Transition &) = delete;
    /// Copy assignement is forbidden.
    Transition &operator=(const Transition &) = delete;
};

}  // namespact automata
}  // namespact spaction

#endif  // defined SPACTION_INCLUDE_CLTLTRANSLATOR_H_
