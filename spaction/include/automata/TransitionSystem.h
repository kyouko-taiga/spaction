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

#include <typeinfo>

namespace spaction {
namespace automata {

template<typename Q, typename S> class Transition;

template<typename Q, typename S> class TransitionSystem {
protected:
    class _TransitionIterator;
    class _StateIterator;
    class StateWrapper;
    class StateContainer;
    class SuccessorContainer;
    class PredecessorContainer;

public:
    typedef _TransitionIterator TransitionIterator;
    typedef _StateIterator StateIterator;

    virtual ~TransitionSystem() { }

    virtual void add_state(const Q &state) = 0;
    virtual void remove_state(const Q &state) = 0;
    virtual bool has_state(const Q &state) const = 0;

    virtual StateWrapper operator()(const Q &state) { return StateWrapper(this, state); }
    virtual StateContainer states() { return StateContainer(this); }

    virtual Transition<Q,S> *add_transition(const Q &source, const Q &sink, const S &label) = 0;
    virtual void remove_transition(const Q &source, const Q &sink, const S &label) = 0;

protected:
    class TransitionBaseIterator {
    public:
        virtual ~TransitionBaseIterator() { }

        bool operator!=(const TransitionBaseIterator& rhs) const {
            // unfortunately, using rtti here is probably better than other alternative
            // see http://stackoverflow.com/questions/11332075
            return (typeid(*this) != typeid(rhs)) or !is_equal(rhs);
        }

        virtual bool is_equal(const TransitionBaseIterator& rhs) const = 0;

        virtual Transition<Q,S>* operator*() = 0;
        virtual const TransitionBaseIterator& operator++() = 0;
    };

    class StateBaseIterator {
    public:
        virtual ~StateBaseIterator() { }

        bool operator!=(const StateBaseIterator& rhs) const {
            // unfortunately, using rtti here is probably better than other alternative
            // see http://stackoverflow.com/questions/11332075
            return (typeid(*this) != typeid(rhs)) or !is_equal(rhs);
        }

        virtual bool is_equal(const StateBaseIterator& rhs) const = 0;

        virtual Q operator*() = 0;
        virtual const StateBaseIterator& operator++() = 0;
    };

    class StateWrapper {
    public:
        explicit StateWrapper(TransitionSystem *transition_system, const Q& state) :
        _transition_system(transition_system), _state(state) {}

        TransitionSystem *transition_system() const { return _transition_system; }
        const Q &state() const { return _state; }

        SuccessorContainer successors() {
            return SuccessorContainer(this);
        }

        SuccessorContainer successors(const S& label) {
            return SuccessorContainer(this, &label);
        }

        /// @note This method is not implemented yet.
        PredecessorContainer predecessors();
        /// @note This method is not implemented yet.
        PredecessorContainer predecessors(const S& label);

    private:
        TransitionSystem *_transition_system;
        const Q _state;
    };

    class _TransitionIterator {
    public:
        explicit _TransitionIterator(TransitionBaseIterator *base_iterator) :
            _base_iterator(base_iterator) { }
        virtual ~_TransitionIterator() { delete _base_iterator; }

        bool operator!=(const _TransitionIterator& rhs) const {
            return *(_base_iterator) != *(rhs._base_iterator);
        }

        Transition<Q,S>* operator*() { return **_base_iterator; }

        const _TransitionIterator& operator++() {
            ++(*_base_iterator);
            return *this;
        }

    private:
        TransitionBaseIterator *_base_iterator;
    };

    class _StateIterator {
    public:
        explicit _StateIterator(StateBaseIterator *base_iterator) :
        _base_iterator(base_iterator) { }
        virtual ~_StateIterator() { delete _base_iterator; }

        bool operator!=(const _StateIterator& rhs) const {
            return *(_base_iterator) != *(rhs._base_iterator);
        }

        Q operator*() { return **_base_iterator; }

        const _StateIterator& operator++() {
            ++(*_base_iterator);
            return *this;
        }

    private:
        StateBaseIterator *_base_iterator;
    };

    class StateContainer {
    public:
        StateContainer(TransitionSystem<Q,S> *ts): _ts(ts) {}
        virtual ~StateContainer() {}

        _StateIterator begin() const {
            return _StateIterator(_ts->_state_begin());
        }
        _StateIterator end() const {
            return _StateIterator(_ts->_state_end());
        }

    private:
        TransitionSystem<Q,S> *_ts;
    };

    class RelationshipContainer {
    public:
        explicit RelationshipContainer(StateWrapper *state_wrapper, const S* label=nullptr) :
            _state_wrapper(state_wrapper), _label(label) { }
        virtual ~RelationshipContainer() { }

        const StateWrapper *state() const { return _state_wrapper; }
        const S *label() const { return _label; }

        virtual _TransitionIterator begin() const = 0;
        virtual _TransitionIterator end() const = 0;

    protected:
        StateWrapper *_state_wrapper;
        const S *_label;
    };

    class SuccessorContainer : public RelationshipContainer {
    public:
        explicit SuccessorContainer(StateWrapper *state_wrapper, const S* label=nullptr) :
            RelationshipContainer(state_wrapper, label) { }

        _TransitionIterator begin() const {
            TransitionSystem<Q,S> *ts = this->_state_wrapper->transition_system();
            return _TransitionIterator(ts->_successor_begin(this->_state_wrapper->state(),
                                                            this->_label));
        }

        _TransitionIterator end() const {
            TransitionSystem<Q,S> *ts = this->_state_wrapper->transition_system();
            return _TransitionIterator(ts->_successor_end(this->_state_wrapper->state()));
        }
    };

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

    virtual TransitionBaseIterator *_successor_begin(const Q &state, const S *label) = 0;
    virtual TransitionBaseIterator *_successor_end(const Q &state) = 0;

    virtual TransitionBaseIterator *_predecessor_begin(const Q &state, const S *label) = 0;
    virtual TransitionBaseIterator *_predecessor_end(const Q &state) = 0;

    virtual StateBaseIterator *_state_begin() = 0;
    virtual StateBaseIterator *_state_end() = 0;
};

/// Class that represents a transition in a TransitionSystem.
template <typename Q, typename S> class Transition {
public:
    const Q &source() const { return _source; }
    const Q &sink()   const { return _sink; }
    const S &label()  const { return _label; }

protected:
    friend class TransitionSystem<Q,S>;

    const Q _source;
    const Q _sink;
    const S _label;

    explicit Transition(const Q &source, const Q &sink, const S &label) :
        _source(source), _sink(sink), _label(label) { }

private:
    /// Copy construction is forbidden.
    Transition(const Transition &) = delete;
    /// Copy assignement is forbidden.
    Transition &operator=(const Transition &) = delete;
};

}  // namespace automata
}  // namespace spaction

#endif  // defined SPACTION_INCLUDE_CLTLTRANSLATOR_H_
