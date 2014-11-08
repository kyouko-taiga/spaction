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

#ifndef SPACTION_INCLUDE_AUTOMATA_TRANSITIONSYSTEM_H_
#define SPACTION_INCLUDE_AUTOMATA_TRANSITIONSYSTEM_H_

#include <typeinfo>

namespace spaction {
namespace automata {

template<typename Q, typename S> class Transition;
template<typename T> class ControlBlock;
template<typename Q, typename S> class TransitionPtr;

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

    explicit TransitionSystem(ControlBlock<Transition<Q, S>> *cb): _control_block(cb) {}
    virtual ~TransitionSystem() { delete _control_block; }

    virtual void add_state(const Q &state) = 0;
    virtual void remove_state(const Q &state) = 0;
    virtual bool has_state(const Q &state) const = 0;

    virtual StateWrapper operator()(const Q &state) { return StateWrapper(this, state); }
    virtual StateContainer states() { return StateContainer(this); }

    virtual const Transition<Q, S> *add_transition(const Q &source, const Q &sink, const S &label) = 0;
    virtual void remove_transition(const Q &source, const Q &sink, const S &label) = 0;

    virtual ControlBlock<Transition<Q, S>> *get_control_block() const { return _control_block; }

 protected:
    ControlBlock<Transition<Q, S>> *_control_block;

    class TransitionBaseIterator {
     public:
        virtual ~TransitionBaseIterator() { }

        bool operator!=(const TransitionBaseIterator& rhs) const {
            // unfortunately, using rtti here is probably better than other alternative
            // see http://stackoverflow.com/questions/11332075
            return (typeid(*this) != typeid(rhs)) or !is_equal(rhs);
        }

        virtual bool is_equal(const TransitionBaseIterator& rhs) const = 0;
        virtual TransitionBaseIterator *clone() const = 0;

        virtual TransitionPtr<Q, S> operator*() = 0;
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
        virtual StateBaseIterator *clone() const = 0;

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

        // copy constructor
        _TransitionIterator(const _TransitionIterator &other) :
            _base_iterator(other._base_iterator->clone()) { }

        // copy assignment operator
        _TransitionIterator &operator=(const _TransitionIterator &other) {
            if (this != &other) {
                delete _base_iterator;
                _base_iterator = other._base_iterator->clone();
            }
            return *this;
        }

        bool operator!=(const _TransitionIterator& rhs) const {
            return *(_base_iterator) != *(rhs._base_iterator);
        }

        /// Returns a pointer to a transition.
        /// @remark This pointer is only valid during the lifetime of the related TransitionSystem.
        ///         When it goes out of scope, this pointer becomes dangling, dereferencing it will
        ///         cause undefined behavior.
        ///         Note that the pointed Transition is const, so consider TransitionPtr<Q,S> as if
        ///         of type 'const Transition<Q,S> *'
        TransitionPtr<Q, S> operator*() { return **_base_iterator; }

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

        _StateIterator(const _StateIterator &other) :
            _base_iterator(other._base_iterator->clone()) { }

        _StateIterator &operator=(const _StateIterator &other) {
            if (this != &other) {
                delete _base_iterator;
                _base_iterator = other._base_iterator->clone();
            }
            return *this;
        }

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
        StateContainer(TransitionSystem<Q, S> *ts): _ts(ts) {}
        virtual ~StateContainer() {}

        _StateIterator begin() const {
            return _StateIterator(_ts->_state_begin());
        }
        _StateIterator end() const {
            return _StateIterator(_ts->_state_end());
        }

     private:
        TransitionSystem<Q, S> *_ts;
    };

    class RelationshipContainer {
     public:
        explicit RelationshipContainer(StateWrapper *state_wrapper, const S* label = nullptr) :
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
        explicit SuccessorContainer(StateWrapper *state_wrapper, const S* label = nullptr) :
            RelationshipContainer(state_wrapper, label) { }

        _TransitionIterator begin() const {
            TransitionSystem<Q, S> *ts = this->_state_wrapper->transition_system();
            return _TransitionIterator(ts->_successor_begin(this->_state_wrapper->state(),
                                                            this->_label));
        }

        _TransitionIterator end() const {
            TransitionSystem<Q, S> *ts = this->_state_wrapper->transition_system();
            return _TransitionIterator(ts->_successor_end(this->_state_wrapper->state()));
        }
    };

    /// Internal method to build transitions.
    /// @remarks
    ///     Instances of the class Transition shouldn't be constructed outside a TransitionSystem.
    ///     Subclasses may use this method within their implementation of `add_transition` to
    ///     actually create the Transition objects.
    virtual Transition<Q, S> *_make_transition(const Q &source, const Q &sink, const S &label) {
        if (!has_state(source) or !has_state(sink))
            return nullptr;
        return new Transition<Q, S>(source, sink, label);
    }
    /// Internal method to destroy transitions.
    /// @remarks
    ///     TransitionSystem being being responsible for the construction of Transition, it is also
    ///     responsible for their destruction.
    ///     Subclasses may use this method within their implementation of `remove_transition` to
    ///     actually delete the Transition objects.
    virtual void _delete_transition(const Transition<Q, S> *t) {
        delete t;
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
    /// A Transition may only be built and destroyed from the related TransitionSystem.
    friend class TransitionSystem<Q, S>;

    const Q _source;
    const Q _sink;
    const S _label;

    /// Constructor
    explicit Transition(const Q &source, const Q &sink, const S &label) :
        _source(source), _sink(sink), _label(label) { }

    /// Destructor
    ~Transition() {}

 private:
    /// Copy construction is forbidden.
    Transition(const Transition &) = delete;
    /// Copy assignement is forbidden.
    Transition &operator=(const Transition &) = delete;
};

/// making `operator==` non member allows easy specialization if needed (no class specialization)
template<typename Q, typename S>
bool operator==(const spaction::automata::Transition<Q, S> &lhs,
                const spaction::automata::Transition<Q, S> &rhs) {
    return lhs.source() == rhs.source() and lhs.sink() == rhs.sink() and lhs.label() == rhs.label();
}

/// Control block interface.
/// Acts as the real memory manager: pass it newly acquired pointers,
/// and tell it to destroy the pointer when ref count reaches 0.
template<typename T>
class ControlBlock {
public:
    virtual ~ControlBlock() { }

    /// Called when an object starts being managed.
    virtual void declare(const T *t) = 0;
    /// Called when an object is no longer managed.
    virtual void release(const T *t) = 0;
};

/// The structure actually stored by the smart pointer (see below).
template<typename T>
struct SmartBlock {
    // are all default ctors, dtor, copy and move semantics OK?
    std::size_t _refcount;
    const T * const _pointer;
    ControlBlock<T> * const _control;

    /// constructor
    explicit SmartBlock(size_t r, const T *p, ControlBlock<T> *cb):
    _refcount(r), _pointer(p), _control(cb) {
        // declare the newly managed pointer to the controller
        _control->declare(_pointer);
    }
    /// destructor
    ~SmartBlock() {
        _control->release(_pointer);
    }
};


/// A smart pointer class to handle class Transition outside of a TransitionSystem
/// It more or less acts as a std::shared_ptr with a privileged owner. When this privileged owner
/// goes out of scope, the pointer is destroyed, and other owners are left with dangling pointers.
/// @todo specialize std::swap, std::hash, operator<, operator==
/// @todo use nothrow when required
/// @todo what about thread-safety?
template<typename Q, typename S>
class TransitionPtr {
 public:
    /// @todo restrict constructor visibility?
    explicit TransitionPtr(const Transition<Q, S> *t, ControlBlock<Transition<Q, S>> *cb):
        _data(new SmartBlock<Transition<Q, S>>(1, t, cb)) { }

    /// destructor
    ~TransitionPtr() {
        if (decr()) {
            delete _data;
        }
    }

    /// copy semantics
    explicit TransitionPtr(const TransitionPtr &other): _data(other._data) {
        incr();
    }

    TransitionPtr & operator=(const TransitionPtr &other) {
        if (this != &other) {
            if (_data != other._data) {
                if (decr()) {
                    delete _data;
                }
            }
            _data = other._data;
            incr();
        }
        return *this;
    }

    /// move semantics
    TransitionPtr(TransitionPtr &&other): _data(other._data) {
        other._data = nullptr;
    }

    TransitionPtr & operator=(TransitionPtr &&other) {
        if (decr() and _data != other._data) {
            delete _data;
            _data = other._data;
        }
        other._data = nullptr;
        return *this;
    }

    /// Pointer interface: dereference
    const Transition<Q, S> &operator*() const { return *_data->_pointer; }
    /// Pointer interface: member access
    const Transition<Q, S> *operator->() const { return _data->_pointer; }
    /// Pointer interface: cast to bool
    explicit operator bool() const { return _data->_pointer; }

    /// Utility functions
    std::size_t hash() const;
    bool operator< (const TransitionPtr &) const;
    bool operator==(const TransitionPtr &) const;

 private:
    SmartBlock<Transition<Q, S>> *_data;

    inline void incr() { if (_data) ++_data->_refcount; }
    inline bool decr() { return _data and not --_data->_refcount; }
};

}  // namespace automata
}  // namespace spaction

#endif  // SPACTION_INCLUDE_AUTOMATA_TRANSITIONSYSTEM_H_
