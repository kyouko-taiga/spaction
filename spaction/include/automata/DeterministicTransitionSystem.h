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

#ifndef SPACTION_INCLUDE_DETERMINISTICTRANSITIONSYSTEM_H_
#define SPACTION_INCLUDE_DETERMINISTICTRANSITIONSYSTEM_H_

#include <cassert>
#include <unordered_map>

#include "TransitionSystem.h"

namespace spaction {
namespace automata {

template<typename Q, typename S>
class DeterministicTransitionSystem : public TransitionSystem<Q,S> {

    class BaseIterator : public TransitionSystem<Q,S>::BaseIterator {
    public:
        explicit BaseIterator(DeterministicTransitionSystem<Q,S> *transition_system,
                              const Q *q, const S *s) : _transition_system(transition_system) {
            if(s) {
                _it = _transition_system->_graph[*q].end();
                _transition = _transition_system->_graph[*q][*s];
            } else {
                _it = _transition_system->_graph[*q].begin();
                _transition = _it->second;
            }
            _end = _transition_system->_graph[*q].end();
        }

        /// Constructor that builds the end iterator.
        BaseIterator(typename std::unordered_map<S, Transition<Q,S>*>::iterator end) :
            _it(end), _end(end), _transition(nullptr) { }

        virtual bool is_equal(const typename TransitionSystem<Q,S>::BaseIterator& rhs)
        const {
            const BaseIterator& bi = static_cast<const BaseIterator&>(rhs);
            return _transition == bi._transition;
        }

        virtual Transition<Q,S>* operator*() {
            return _transition;
        }

        virtual const typename TransitionSystem<Q,S>::BaseIterator& operator++() {
            // We need to compare _it twice, since it can have been set to
            // _end if the transition label was supplied when creating the
            // iterator.
            if(_it != _end) { ++_it; }

            if(_it != _end) {
                _transition = _it->second;
            } else {
                _transition = nullptr;
            }
            return *this;
        }

    private:
        typename std::unordered_map<S, Transition<Q,S>*>::iterator _it, _end;

        DeterministicTransitionSystem<Q,S> *_transition_system;
        Transition<Q,S> *_transition;
    };

public:
    virtual void add_state(const Q &state) {
        if (_graph.count(state) > 0) return;
        _graph[state];
    }

    virtual void remove_state(const Q &state) { }

    virtual bool has_state(const Q &state) const {
        return _graph.count(state) > 0;
    }

    virtual Transition<Q,S> *add_transition(const Q &source, const Q &sink, const S &label) {
        assert(has_state(source) and has_state(sink));
        
        Transition<Q,S> *t = this->_make_transition(source, sink, label);
        _graph[source][label] = t;
        return t;
    }

    virtual void remove_transition(const Q &source, const Q &sink, const S &label) { }

protected:
    std::unordered_map<Q, std::unordered_map<S, Transition<Q,S>*>> _graph;

    virtual typename TransitionSystem<Q,S>::BaseIterator *_successor_begin(const Q &state,
                                                                           const S *label) {
        return new BaseIterator(this, &state, label);
    }

    virtual typename TransitionSystem<Q,S>::BaseIterator *_successor_end(const Q &state) {
        return new BaseIterator(_graph[state].end());
    }

    /// @note This method is not implemented yet.
    virtual typename TransitionSystem<Q,S>::BaseIterator *_predecessor_begin(const Q &state,
                                                                             const S *label) {
        return nullptr;
    }

    /// @note This method is not implemented yet.
    virtual typename TransitionSystem<Q,S>::BaseIterator *_predecessor_end(const Q &state) {
        return nullptr;
    }
};

}  // namespace automata
}  // namespace spaction

#endif
