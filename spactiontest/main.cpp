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

#include <iostream>
#include <string>

#include "CltlFormula.h"
#include "CltlFormulaFactory.h"
#include "spotcheck.h"

#include "automata/CltlTranslator.h"
#include "automata/RegisterAutomaton.h"

// this file is strongly inspired from spot/iface/dve2/dve2check.cc

/// Runs a simple test of the Cost Register Automaton library.
/// @remarks
///     This function creates a cost register automaton, runs it over a string of character and
///     prints the result of its partial final cost function on the console. The built automaton
///     corresponds to one of the examples of CRA from the paper of Alur et al, "Regular Functions
///     and Cost Register Automata", 2013
void test_cost_register_automata(const std::string &str = "aabaaacba") {
    // create a cost register automaton with 2 registers
    spaction::automata::RegisterAutomaton<char> automaton(2);

    // build the automaton state
    automaton.add_state("q0", true);

    // build the automaton transitions:
    typedef spaction::automata::RegisterAutomatonTransition<char> T;
    T *ta = automaton.add_transition("q0", "q0", 'a');
    T *tb = automaton.add_transition("q0", "q0", 'b');
    T *tc = automaton.add_transition("q0", "q0", 'c');

    // set the transition operations
    using spaction::automata::Register;
    using spaction::automata::Registers;
    using spaction::automata::RegisterOperation;

    ta->set_register_operation(0, new RegisterOperation([](const Registers& regs) {
        return regs[0] + 1; }));  // a / x = x + 1
    ta->set_register_operation(1, new RegisterOperation([](const Registers& regs) {
        return regs[1] + 1; }));  // a / y = y + 1

    tb->set_register_operation(1, new RegisterOperation([](const Registers& regs) {
        return regs[1] + 1; }));  // b / y = y + 1

    tc->set_register_operation(0, new RegisterOperation([](const Registers& regs) {
        return regs[1] + 1; }));  // c / x = y + 1
    tc->set_register_operation(1, new RegisterOperation([](const Registers& regs) {
        return regs[1] + 1; }));  // c / y = y + 1

    // run the automaton on a string
    for (std::string::const_iterator it = str.begin(); it != str.end(); ++it) {
        automaton.update(*it);
    }
    std::cout << "μ(q0) = " << automaton.register_value(0) << std::endl;
}

int main(int argc, char* argv[]) {
    test_cost_register_automata();

    if (argc != 2) {
        std::cerr << "wrong number of arguments" << std::endl;
        std::cerr << "usage: spaction model.dve" << std::endl;
        return 1;
    }

    // a test formula: (a + b) * c * (d + a) == acd + ac + bcd + abc
    spaction::CltlFormulaFactory f;
    spaction::CltlFormulaPtr k = f.make_and(f.make_and(f.make_or(f.make_atomic("a"),
                                                                 f.make_atomic("b")),
                                                       f.make_atomic("c")),
                                            f.make_or(f.make_atomic("d"), f.make_atomic("a")));

    std::cout << "input: " << k->dump() << std::endl;
    std::cout << "nnf:   " << k->to_nnf()->dump() << std::endl;
    std::cout << "dnf:   " << k->to_dnf()->dump() << std::endl;

    // a test formula: G(a + b), and its equivalent automaton
    k = f.make_release(f.make_constant(false), f.make_or(f.make_atomic("a"), f.make_atomic("b")));
    spaction::automata::CltlTranslator translator(k);
    translator.build_automaton();

    // a test formula
    // \todo atoms are not properly deleted here
    k = f.make_costuntil(f.make_atomic("P_0.wait"), f.make_atomic("P_0.CS"));

    // G(wait -> F CS)
    /* cltl_formula *f = cltl_factory::make_not(cltl_factory::make_globally(
     * cltl_factory::make_imply(cltl_factory::make_atomic("P_0.wait"),
     *  cltl_factory::make_finally(
     *  cltl_factory::make_atomic("P_0.CS"))))); */

    std::cout << k->dump() << std::endl;

    int bound = spaction::find_bound_min(k, argv[1]);
    std::cout << "the bound for " << k->dump() << " is " << bound << std::endl;

    return 0;
}
