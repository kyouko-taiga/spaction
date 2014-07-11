//
//  CostRegisterAutomata.h
//  spaction
//
//  Created by Dimitri Racordon on 11/07/14.
//  Copyright (c) 2014 University of Geneva. All rights reserved.
//

#ifndef SPACTION_INCLUDE_COSTREGISTERAUTOMATA_H_
#define SPACTION_INCLUDE_COSTREGISTERAUTOMATA_H_

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace spaction {
namespace cra {

template<typename Sigma> class Transition;

typedef unsigned Register;
typedef std::vector<Register> Registers;

class State {
public:
    explicit State(const std::string &name) : _name(name) {}
    const std::string &name() const { return _name; }
    
protected:
    const std::string _name;
};

template<typename  Sigma>
class CostRegisterAutomata {
public:
    explicit CostRegisterAutomata(std::size_t num_registers) :
        _initial_state(nullptr), _current_state(nullptr),
        _registers(num_registers, 0) {
    }

    /// Adds a state to the automata.
    /// @warning
    ///     If a state with the same name has already been added before this method is called, it
    ///     will silently replace the existing state with the one pointed by `state`, and update
    ///     transitions for which the former state was a source and/or a sink.
    State *add_state(State *state, bool initial=false) {
        bool update_transitions = _states.find(state->name()) != _states.end();
        _states[state->name()] = state;

        if (update_transitions) {
            // update transitions whose source and/or sink was the former state
            for (auto& transition: _transitions) {
                if (_transitions->_source->name() == state->name()) _transitions->_source = state;
                if (_transitions->_sink->name() == state->name()) _transitions->_sink = state;
            }

            // update current state if needed
            if (_current_state->name() == state->name()) _current_state = state;
        }
    }

    Transition<Sigma> *add_transition(State *source, State *sink, const Sigma &symbol);
    Transition<Sigma> *add_transition(const std::string &source, const std::string &sink,
                                      const Sigma &symbol);

    State *update(Sigma symbol);

protected:
    State *_initial_state;
    State *_current_state;
    std::unordered_map<std::string, State*> _states;
    std::unordered_map<State*, std::vector<Transition<Sigma>*>> _transitions;

    Registers _registers;
};

template<typename Sigma>
class Transition {
public:

    State *source()             { return _source; }
    State *sink()               { return _sink; }
    const Sigma &symbol() const { return _symbol; }

    void set_register_operation(Register reg, std::function<Register(const Registers&)> lambda) {
        _operations[reg] = lambda;
    }

    std::function<Register(const Registers&)> &get_register_operation(Register reg) {
        return _operations[reg];
    }

protected:
    friend class CostRegisterAutomata<Sigma>;

    State *_source;
    State *_sink;
    const Sigma _symbol;

    std::unordered_map<std::size_t, std::function<Register(const Registers&)>> _operations;

    explicit Transition(State *source, State *sink, const Sigma &symbol) :
        _source(source), _sink(sink), _symbol(symbol) {
    }

    /// Disallows copy constructor.
    Transition(Transition const&) = 0;
    /// Disallows copy assignement.
    void operator=(Transition const&) = 0;
};

}  // namespact cra
}  // namespace spaction

#endif  // defined SPACTION_INCLUDE_COSTREGISTERAUTOMATA_H_
