//
//  CostRegisterAutomaton.h
//  spaction
//
//  Created by Dimitri Racordon on 11/07/14.
//  Copyright (c) 2014 University of Geneva. All rights reserved.
//

#ifndef SPACTION_INCLUDE_COSTREGISTERAUTOMATON_H_
#define SPACTION_INCLUDE_COSTREGISTERAUTOMATON_H_

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace spaction {
namespace cra {

typedef unsigned Register;
typedef std::vector<Register> Registers;

typedef std::function<Register(const Registers&)> RegisterOperation;

template<typename Sigma> class Transition;

template<typename  Sigma>
class CostRegisterAutomaton {
public:
    explicit CostRegisterAutomaton(std::size_t num_registers) :
        _initial_state(""), _current_state(""), _registers(num_registers, 0) {
    }

    /// Adds a state to the automata.
    /// @warning
    ///     States names are expected to be unique in an automaton. To ensure this property, any
    ///     call to this method will be silently ignore if `name` already designates a state.
    void add_state(const std::string &name, bool initial=false) {
        if(_graph.count(name) > 0)
            return;

        _graph[name];
        if(initial) _initial_state = name;
    }

    bool has_state(const std::string &name) const { return _graph.count(name) > 0; }
    Register register_value(std::size_t reg)      { return _registers[reg]; }

    Transition<Sigma> *add_transition(const std::string &source, const std::string &sink,
                                      const Sigma &symbol) {
        if(!has_state(source) || !has_state(sink))
            return nullptr;

        Transition<Sigma> *t = new Transition<Sigma>(source, sink, symbol, _registers.size());
        _graph[source][symbol] = t;
        return t;
    }

    const std::string &update(const Sigma &symbol) {
        if (_current_state.empty()) _current_state = _initial_state;

        // retrieve the outgoind transition
        Transition<Sigma> *t = _graph[_current_state].at(symbol);

        // update registers
        Registers updated_registers(_registers.size());
        for (std::size_t i = 0; i < _registers.size(); ++i) {
            auto operation = t->register_operation(i);
            updated_registers[i] = operation ? (*operation)(_registers) : _registers[i];
        }
        _registers = std::move(updated_registers);

        // update current state
        _current_state = t->sink();
        return _current_state;
    }

protected:
    std::string _initial_state;
    std::string _current_state;
    std::unordered_map<std::string, std::unordered_map<Sigma, Transition<Sigma>*>> _graph;

    Registers _registers;
};

template<typename Sigma>
class Transition {
public:
    const std::string &source() const { return _source; }
    const std::string &sink()   const { return _sink; }
    const Sigma &symbol()       const { return _symbol; }

    /// Sets the operation to be performed on a register when the transition is fired.
    /// @remarks
    ///     Whenever this transition is fired, it will call all registered operations for each
    ///     register of the automaton. If there isn't any operation set on a particular register,
    ///     the latter will remain unchanged. Note that only one operation can be registered for a
    ///     given register; calling this method will silently replace any previously registered
    ///     operation, for a given register.
    ///
    /// @param reg The index of the register for which the operation will be set.
    /// @param operation A function of the form Registers -> Register. It will be called with a
    ///     vector of all the values of the automaton register, and is expected to return the new
    ///     value of the register it is set for.
    void set_register_operation(std::size_t reg, std::shared_ptr<RegisterOperation> operation) {
        _operations[reg] = operation;
    }

    /// @copydoc set_register_operation(std::size_t, std::shared_ptr<RegisterOperation>)
    void set_register_operation(std::size_t reg, RegisterOperation *operation) {
        set_register_operation(reg, std::shared_ptr<RegisterOperation>(operation));
    }

    /// Retrieves the operation defined for a particular register.
    std::shared_ptr<RegisterOperation> register_operation(std::size_t reg) {
        return _operations[reg];
    }

protected:
    friend class CostRegisterAutomaton<Sigma>;

    const std::string &_source;
    const std::string &_sink;
    const Sigma _symbol;

    std::vector<std::shared_ptr<RegisterOperation>> _operations;

    explicit Transition(const std::string &source, const std::string &sink, const Sigma &symbol,
                        std::size_t num_registers) :
        _source(source), _sink(sink), _symbol(symbol), _operations(num_registers, nullptr) {
    }

    /// Disallows copy constructor.
    Transition(Transition const&);
    /// Disallows copy assignement.
    void operator=(Transition const&);
};

}  // namespact cra
}  // namespace spaction

#endif  // defined SPACTION_INCLUDE_COSTREGISTERAUTOMATON_H_