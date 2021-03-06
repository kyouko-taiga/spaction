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

#ifndef SPACTION_INCLUDE_AUTOMATA_UNDETERMINISTICTRANSITIONSYSTEM_H_
#define SPACTION_INCLUDE_AUTOMATA_UNDETERMINISTICTRANSITIONSYSTEM_H_

#include <cassert>
#include <unordered_map>

#include "TransitionSystem.h"
#include "TransitionSystemPrinter.h"

namespace spaction {
namespace automata {

/// A dumb ControlBlock implementation, that does nothing.
/// The real storage is in fact the UndeterministicTransitionSystem.
template<typename T>
class DumbControlBlock : public ControlBlock<T> {
 public:
    explicit DumbControlBlock() {}
    ~DumbControlBlock() {}

    void declare(const T *) override { }
    void release(const T *) override { }
};

template<typename Q, typename S>
class UndeterministicTransitionSystem : public TransitionSystem<Q, S> {
    class TransitionBaseIterator : public TransitionSystem<Q, S>::TransitionBaseIterator {
     public:
        explicit TransitionBaseIterator(UndeterministicTransitionSystem<Q, S> *transition_system,
                                        const Q *q, const S *s) :
        _transition_system(transition_system) {
            _labeled = s != nullptr;
            if (_labeled) {
                _it = _transition_system->_graph[*q].find(*s);
            } else {
                _it = _transition_system->_graph[*q].begin();
            }
            _end = _transition_system->_graph[*q].end();
            if (_it == _end) {
                _transition_it = typename std::vector<const Transition<Q, S>*>::iterator();
            } else {
                _transition_it = _it->second.begin();
            }
        }

        /// Constructor that builds the end iterator.
        explicit TransitionBaseIterator(typename std::unordered_map<S, std::vector<const Transition<Q, S>*>>::iterator end) :
            _it(end), _end(end),
            _transition_it(typename std::vector<const Transition<Q, S>*>::iterator()) { }

        virtual bool
        is_equal(const typename TransitionSystem<Q, S>::TransitionBaseIterator& rhs) const {
            const TransitionBaseIterator& bi = static_cast<const TransitionBaseIterator&>(rhs);
            bool   b = _it == bi._it;
            return b and ((_it == _end) or (_transition_it == bi._transition_it));
        }

        virtual TransitionPtr<Q, S> operator*() {
            assert(_it != _end);
            assert(_transition_it != _it->second.end());
            // @todo why does the following assertion fail?
            // assert(_transition_it != (typename std::vector<Transition<Q,S>*>::iterator()));
            return TransitionPtr<Q, S>(*_transition_it, _transition_system->get_control_block());
        }

        virtual const typename TransitionSystem<Q, S>::TransitionBaseIterator& operator++() {
            // increment the transition iterator
            if (++_transition_it != _it->second.end()) {
                return *this;
            }

            // increment the label iterator, unless we were constructed for a specific label
            if ((++_it != _end) and !_labeled) {
                _transition_it = _it->second.begin();
                /// @note
                ///     this assert holds if `remove_state` and `remove_transition` behave correctly
                assert(_transition_it != _it->second.end());
            } else {
                // if `_labeled`, ensure that `_it` is set to `_end` for proper comparison with end
                _it = _end;
                _transition_it = typename std::vector<const Transition<Q, S>*>::iterator();
            }

            return *this;
        }

        virtual typename TransitionSystem<Q, S>::TransitionBaseIterator *clone() const {
            return new TransitionBaseIterator(*this);
        }

     private:
        typename std::unordered_map<S, std::vector<const Transition<Q, S>*>>::iterator _it, _end;
        typename std::vector<const Transition<Q, S>*>::iterator _transition_it;

        UndeterministicTransitionSystem<Q, S> *_transition_system;
        bool _labeled;
    };

    class StateBaseIterator : public TransitionSystem<Q, S>::StateBaseIterator {
     public:
        explicit StateBaseIterator(UndeterministicTransitionSystem<Q, S> *transition_system) :
            _transition_system(transition_system) {
            _it = _transition_system->_graph.begin();
            _end = _transition_system->_graph.end();
        }

        /// Constructor that builds the end iterator.
        explicit StateBaseIterator(typename std::unordered_map<Q, std::unordered_map<S, std::vector<const Transition<Q, S>*>>>::iterator end) :
            _it(end), _end(end) { }

        virtual bool is_equal(const typename TransitionSystem<Q, S>::StateBaseIterator& rhs) const {
            const StateBaseIterator& bi = static_cast<const StateBaseIterator&>(rhs);
            return _it == bi._it;
        }

        virtual Q operator*() {
            return _it->first;
        }

        virtual const typename TransitionSystem<Q, S>::StateBaseIterator& operator++() {
            ++_it;
            return *this;
        }

        virtual typename TransitionSystem<Q, S>::StateBaseIterator *clone() const {
            return new StateBaseIterator(*this);
        }

     private:
        typename std::unordered_map<Q, std::unordered_map<S, std::vector<const Transition<Q, S>*>>>::iterator _it, _end;

        UndeterministicTransitionSystem<Q, S> *_transition_system;
    };

 public:
    explicit UndeterministicTransitionSystem():
        TransitionSystem<Q, S>(new DumbControlBlock<Transition<Q, S>>()) {}

    ~UndeterministicTransitionSystem() {
        for (auto &it : _graph) {
            for (auto &jt : it.second) {
                for (auto &trans : jt.second) {
                    this->_delete_transition(trans);
                }
            }
        }
    }

    virtual void add_state(const Q &state) {
        if (_graph.count(state) > 0) return;
        _graph[state];
    }

    /// @note
    ///     this method should ensure that a label with no successors does not appear in the maps
    virtual void remove_state(const Q &state) {
        // @todo
    }

    virtual bool has_state(const Q &state) const {
        return _graph.count(state) > 0;
    }

    virtual const Transition<Q, S> *add_transition(const Q &source, const Q &sink, const S &label) {
        assert(has_state(source) and has_state(sink));

        const Transition<Q, S> *t = this->_make_transition(source, sink, label);
        auto &v = _graph[source][label];
        auto it = std::find_if(v.begin(), v.end(), [&t](const Transition<Q, S> *o) { return *t == *o; });
        if (it == v.end()) {
            // if the transition is not already stored, add it
            v.push_back(t);
            return t;
        } else {
            // else destroy the temporary and return the stored one
            this->_delete_transition(t);
            return *it;
        }
    }

    /// @note
    ///     this method should ensure that a label with no successors does not appear in the maps
    virtual void remove_transition(const Q &source, const Q &sink, const S &label) {
        // @todo
    }

    virtual void print_state(std::ostream &os, const Q &q) const override {
        PrinterHelper<Q>::print(os, q);
    }
    virtual void print_label(std::ostream &os, const S &s) const override {
        PrinterHelper<S>::print(os, s);
    }

 protected:
    std::unordered_map<Q, std::unordered_map<S, std::vector<const Transition<Q, S>*>>> _graph;

    virtual typename TransitionSystem<Q, S>::TransitionBaseIterator *_successor_begin(const Q &state,
                                                                                      const S *label) {
        return new TransitionBaseIterator(this, &state, label);
    }

    virtual typename TransitionSystem<Q, S>::TransitionBaseIterator *_successor_end(const Q &state) {
        return new TransitionBaseIterator(_graph[state].end());
    }

    /// @note This method is not implemented yet.
    virtual typename TransitionSystem<Q, S>::TransitionBaseIterator *_predecessor_begin(const Q &state,
                                                                                        const S *label) {
        return nullptr;
    }

    /// @note This method is not implemented yet.
    virtual typename TransitionSystem<Q, S>::TransitionBaseIterator *_predecessor_end(const Q &state) {
        return nullptr;
    }

    virtual typename TransitionSystem<Q, S>::StateBaseIterator *_state_begin() {
        return new StateBaseIterator(this);
    }
    virtual typename TransitionSystem<Q, S>::StateBaseIterator *_state_end() {
        return new StateBaseIterator(_graph.end());
    }
};

}  // namespace automata
}  // namespace spaction

#endif  // SPACTION_INCLUDE_AUTOMATA_UNDETERMINISTICTRANSITIONSYSTEM_H_
