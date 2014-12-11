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
#include <unistd.h>

#include "CltlFormula.h"
#include "CltlFormulaFactory.h"
#include "spotcheck.h"

#include "automata/CltlTranslator.h"
#include "automata/CounterAutomaton.h"
#include "automata/CounterAutomatonProduct.h"
#include "automata/TransitionSystemPrinter.h"
#include "automata/UndeterministicTransitionSystem.h"

#include "automata/RegisterAutomaton.h"

#include "cltlparse/public.h"

// this file is strongly inspired from spot/iface/dve2/dve2check.cc

/// Runs a simple test of the CounterAutomaton library.
/// @remarks
///     This function creates a B counter automaton that recognize any word over the alphabet {a,b}
///     where 'b' occurs infinitely often. In addition, the automaton uses its counter to count the
///     largest block of consecutive 'a's.
void test_counter_automata() {
    using spaction::automata::CounterOperation;

    typedef std::string qt;
    typedef char st;

    // create the automaton
    typedef spaction::automata::CounterAutomaton<qt, st,
        spaction::automata::UndeterministicTransitionSystem> Automaton;
    Automaton automaton(1, 1);

    // populate the transition system
    automaton.transition_system()->add_state("q");
    automaton.set_initial_state("q");

    automaton.transition_system()->add_transition("q", "q",
        automaton.make_label('a', {{CounterOperation::kIncrement,
                                    CounterOperation::kCheck}}, std::set<std::size_t>()));

    automaton.transition_system()->add_transition("q", "q",
        automaton.make_label('b', {{CounterOperation::kIncrement,
                                    CounterOperation::kReset}}, {0}));

    spaction::automata::TSPrinter<qt, spaction::automata::CounterLabel<st>> printer(*automaton.transition_system());
    // printer.dump("/tmp/counter.dot");
    printer.dump(std::cout);
}

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

void usage() {
    // @TODO print usage help message
}

void test_product() {
    spaction::CltlFormulaPtr f1 = spaction::cltlparse::parse_formula("\"a\" UN \"b\"");
    spaction::CltlFormulaPtr f2 = spaction::cltlparse::parse_formula("\"c\" UN \"d\"");

    spaction::automata::CltlTranslator t1(f1);
    t1.build_automaton();
    spaction::automata::CltlTranslator t2(f2);
    t2.build_automaton();

    t1.get_automaton().print("aut1.dot");
    t2.get_automaton().print("aut2.dot");

    auto prod = spaction::automata::make_aut_product(t1.get_automaton(), t2.get_automaton(), f1->creator());
    prod.print("prod.dot");

    std::cerr << "product test ended" << std::endl;
}

int main(int argc, char* argv[]) {
//    test_counter_automata();
//    test_product();

    std::string cltl_string = "";
    std::string epsilon_dot_file = "";
    std::string automaton_dot_file = "";
    std::string model_file = "";

    int c;
    while ((c = getopt(argc, argv, "f:e:a:m:")) != -1) {
        switch (c) {
            case 'f':
                cltl_string = optarg;
                break;
            case 'e':
                epsilon_dot_file = optarg;
                break;
            case 'a':
                automaton_dot_file = optarg;
                break;
            case 'm':
                model_file = optarg;
                break;
            default:
                std::cerr << "unknown option " << c << std::endl;
                std::cerr << "abort" << std::endl;
                usage();
                return 1;
        }
    }

    if (cltl_string == "") {
        std::cerr << "no input formula, abort" << std::endl;
        return 1;
    }

    spaction::CltlFormulaPtr f = spaction::cltlparse::parse_formula(cltl_string);
    if (f == nullptr) {
        std::cerr << "parsing went wrong, abort" << std::endl;
        return 1;
    }

    std::cout << "input: " << cltl_string << std::endl;
    std::cout << "nnf:   " << f->to_nnf()->dump() << std::endl;
    std::cout << "dnf:   " << f->to_dnf()->dump() << std::endl;
    std::cout << "the input formula is " << f->dump() << std::endl;

    if (automaton_dot_file != "" or epsilon_dot_file != "") {
        spaction::automata::CltlTranslator translator(f);
        translator.build_automaton();
        if (automaton_dot_file != "") {
            translator.automaton_dot(automaton_dot_file);
            std::cerr << "automaton was printed to file " << automaton_dot_file << std::endl;
        }
        if (epsilon_dot_file != "") {
            translator.epsilon_dot(epsilon_dot_file);
            std::cerr << "epsilon-automaton was printed to file " << epsilon_dot_file << std::endl;
        }
    }

    unsigned int result = spaction::find_bound_max(f, model_file);

    std::cout << "the max bound is " << result << std::endl;

    return 0;
}
