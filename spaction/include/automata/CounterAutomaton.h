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

#ifndef SPACTION_INCLUDE_COUNTERAUTOMATON_H_
#define SPACTION_INCLUDE_COUNTERAUTOMATON_H_

#include <algorithm>
#include <cassert>
#include <ostream>
#include <unordered_map>
#include <vector>

#include "automata/TransitionSystem.h"

namespace spaction {
namespace automata {

enum CounterOperation : unsigned int {
    kIncrement  = 1,
    kCheck      = 2,
    kReset      = 4
};

typedef std::vector<CounterOperation> CounterOperationList;
template<typename S> class CounterLabel;

inline CounterOperation operator|(const CounterOperation &l, const CounterOperation &r) {
    return static_cast<CounterOperation>(static_cast<unsigned int>(l) | static_cast<unsigned int>(r));
}
inline CounterOperation operator&(const CounterOperation &l, const CounterOperation &r) {
    return static_cast<CounterOperation>(static_cast<unsigned int>(l) & static_cast<unsigned int>(r));
}

std::string print_counter_operation(CounterOperation c);

}  // namespace automata
}  // namespace spaction

namespace std {

/// Hash function for CounterLabel type, so it can be used as a key in map-like STL containers.
template<typename S>
struct hash<spaction::automata::CounterLabel<S>> {
    std::size_t operator()(const spaction::automata::CounterLabel<S> &cl) const {
        return cl.hash();
    }
};

}  // namespace std

namespace spaction {
namespace automata {

template<typename Q, typename S, template<typename Q_, typename S_> class TransitionSystemType>
class CounterAutomaton {
public:
    typedef TransitionSystemType<Q, CounterLabel<S>> transition_system_t;
    typedef Transition<Q, CounterLabel<S>>           transition_t;

    explicit CounterAutomaton(std::size_t counters, std::size_t acceptance_sets) :
        _counters(counters, 0), _acceptance_sets(acceptance_sets), _initial_state(nullptr) {

        // static_cast prevents the template from being incompatible
        _transition_system =
            static_cast<TransitionSystem<Q, CounterLabel<S>>*>(new transition_system_t());
    }

    // a convenient default constructor
    // @todo restrict its usage?
    explicit CounterAutomaton(): CounterAutomaton(0, 0) {};

    ~CounterAutomaton() {
        // delete the pointer to the initial state, if ever created
        if (_initial_state)
            delete _initial_state;

        // delete the transition system and its related objects
        _acceptance_sets.clear();
        delete _transition_system;
    }

    inline std::size_t num_counters()        const { return _counters.size(); }
    inline std::size_t num_acceptance_sets() const { return _acceptance_sets.size(); }

    inline transition_system_t *transition_system() {
        return static_cast<transition_system_t*>(this->_transition_system);
    }

    void set_initial_state(const Q &state) {
        assert(_transition_system->has_state(state));

        if (_initial_state)
            delete _initial_state;
        _initial_state = new Q(state);
    }

    inline const Q *initial_state() const { return _initial_state; }

    void add_acceptance_transition(std::size_t set_index, transition_t* transition) {
        _acceptance_sets[set_index].push_back(transition);
    }

    /// Helper method to create transition labels.
    CounterLabel<S> make_label(const S &letter) {
        return CounterLabel<S>(letter, this->num_counters());
    }
    /// Helper method to create transition labels.
    CounterLabel<S> make_label(const S &letter,
                               const std::vector<CounterOperationList> &operations) {
        assert(operations.size() == this->num_counters());
        return CounterLabel<S>(letter, operations);
    }

private:
    TransitionSystem<Q, CounterLabel<S>> *_transition_system;

    std::vector<std::size_t> _counters;
    std::vector<std::vector<transition_t*>> _acceptance_sets;

    const Q *_initial_state;
};

template<typename S> class CounterLabel {
public:
    explicit CounterLabel(const S &letter, std::size_t counters) : _letter(letter),
        _operations(counters), _hash_dirty(true) {
    }

    explicit CounterLabel(const S &letter, const std::vector<CounterOperationList> &operations) :
        _letter(letter), _operations(operations), _hash_dirty(true) {
    }

    bool operator==(const CounterLabel<S>& rhs) const {
        return this->hash() == rhs.hash();
    }

    std::size_t hash() const {
        if (_hash_dirty) {
            _hash_value = std::hash<S>()(_letter);
            for(auto counter : _operations) {
                std::size_t i = 0;
                for(auto operation : counter)
                    i ^= operation;
                _hash_value ^= i;
            }
        }
        return _hash_value;
    }

    inline const S &letter() const { return _letter; }

    inline std::size_t num_counters() const { return _operations.size(); }

    /// Sets a CounterOperation on a counter.
    inline const CounterOperationList counter_operations(std::size_t counter) const {
        return _operations[counter];
    }

    /// Retrieves the operations set for a particular counter.
    /// @remarks
    ///     Counter operations are stored in a FIFO list, and will be applied to a counter in that
    ///     order. This method always adds `operation` at the end of the list.
    void add_counter_operation(std::size_t counter, CounterOperation operation) {
        _operations[counter].push_back(operation);
        _hash_dirty = true;
    }

    /// Removes an operation set for a particular counter.
    /// @remarks
    ///     Counter operations are stored in a FIFO list, and will be applied to a counter in that
    ///     order. This method always erases the first occurence of `operation` within the list.
    void remove_counter_operation(std::size_t counter, CounterOperation operation) {
        CounterOperationList &list = _operations[counter];
        auto it = std::find(list.begin(), list.end(), operation);
        if (it != list.end()) {
            list.erase(it);
        }
    }

private:
    const S _letter;
    std::vector<CounterOperationList> _operations;

    /// This flag indicates if the actual hash function must be computed when hash() is called.
    /// @remarks
    ///     As the hash function is expensive, we use this dirty flag to compute it only when the
    ///     members of the counter label has been modified.
    mutable bool _hash_dirty;
    mutable std::size_t _hash_value;
};

/// Output operator for CounterLabel.
template<typename S>
std::ostream &operator<<(std::ostream &os, const CounterLabel<S>& label) {
    os << label.letter() <<  ":[";
    for (std::size_t i = 0; i < label.num_counters(); ++i) {
        const CounterOperationList &counter = label.counter_operations(i);
        os << "(";
        std::copy(counter.begin(), counter.end(), std::ostream_iterator<int>(os, ","));
        os << "),";
    }
    return os;
}

}  // namespace automata
}  // namespace spaction

#endif
