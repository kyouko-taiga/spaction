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
#include <getopt.h>
#include <string>

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

#include "Logger.h"


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

void usage() {
    std::cerr << "spaction" << std::endl;
    std::cerr << "Mandatory Arguments:" << std::endl;
    std::cerr << "\t-f <formula>, --formula <formula>" << std::endl
        << "\t\tthe CLTL input formula <formula>" << std::endl;
    std::cerr << "\t-m <model>, --model <model>" << std::endl
        << "\t\tthe input model. <model> is the path to the DVE file." << std::endl;
    std::cerr << "Optional Arguments:" << std::endl;
    std::cerr << "\t-s <strat>, --strategy <strat>" << std::endl
        << "\t\tthe strategy to use. Possible values for <strat> are \'direct\' and \'cegar\'." << std::endl
        << "\t\tDefault value is \'direct\'" << std::endl;
    std::cerr << "\t-v <verb>, --verbosity <verb>" << std::endl
        << "\t\tthe verbosity level. <verb> should an integer between 0 and 4." << std::endl
        << "\t\t\t0 logs only fatal errors" << std::endl
        << "\t\t\t1 further logs non-fatal errors" << std::endl
        << "\t\t\t2 further logs warnings" << std::endl
        << "\t\t\t3 further logs general informations [default]" << std::endl
        << "\t\t\t4 further logs debug informations" << std::endl
        << "\t\tIf used without argument, sets to debug (level 4)." << std::endl;
}

int main(int argc, char* argv[]) {

    std::string cltl_string = "";
    std::string model_file = "";
    spaction::BoundSearchStrategy strategy = spaction::BoundSearchStrategy::DIRECT;
    spaction::Logger<std::cerr>::LogLevel log_level = spaction::Logger<std::cerr>::LogLevel::kINFO;

    static struct option long_options[] = {
        /// sets the verbosity level, from 0 (only fatal errors are logged) to 4 (debug info are logged)
        /// the default verbosity level is 3
        /// using the option --verbose without argument sets it to 4 (debug)
        {"verbose",     optional_argument,  0, 'v'},
        /// the CLTL formula to check
        {"formula",     required_argument,  0, 'f'},
        /// the path to a dve file containing the model to check
        {"model",       required_argument,  0, 'm'},
        /// the strategy to use
        ///     valid arguments are 'cegar' and 'direct'
        {"strategy",    required_argument,  0, 's'},
        /// end of array
        {0, 0, 0, 0}
    };

    while (1) {
        int c = getopt_long(argc, argv, "f:m:s:v", long_options, nullptr);
        // no more options to parse
        if (c == -1)
            break;

        switch (c) {
            case 'f':
                cltl_string = optarg;
                break;
            case 'm':
                model_file = optarg;
                break;
            case 's':
                if (std::string("DIRECT") == optarg) {
                    strategy = spaction::BoundSearchStrategy::DIRECT;
                } else if (std::string("CEGAR") == optarg) {
                    strategy = spaction::BoundSearchStrategy::CEGAR;
                } else {
                    spaction::Logger<std::cerr>::instance().error() << "unknown strategy "
                        << optarg << std::endl << "use default strategy instead" << std::endl;
                }
                break;
            case 'v':
                if (optarg)
                    log_level = static_cast<spaction::Logger<std::cerr>::LogLevel>(optarg[0] - 'a');
                else
                    log_level = spaction::Logger<std::cerr>::LogLevel::kDEBUG;
                std::cerr << "loglevel set to " << (static_cast<char>(log_level) + 'a') << std::endl;
                std::cerr << "a set to " << (int)'a' << std::endl;
                break;
            default:
                spaction::Logger<std::cerr>::instance().fatal() << "unknown option " << c << std::endl;
                usage();
                return 1;
        }
    }

    // set the verbosity level
    spaction::Logger<std::cerr>::instance().set_verbose(log_level);

    if (cltl_string == "") {
        spaction::Logger<std::cerr>::instance().fatal() << "no input formula, abort" << std::endl;
        return 1;
    }

    spaction::CltlFormulaPtr f = spaction::cltlparse::parse_formula(cltl_string);
    if (f == nullptr) {
        spaction::Logger<std::cerr>::instance().fatal() << "formula parsing went wrong, abort" << std::endl;
        return 1;
    }

    std::cout << "input: " << cltl_string << std::endl;
    std::cout << "nnf:   " << f->to_nnf()->dump() << std::endl;
    std::cout << "dnf:   " << f->to_dnf()->dump() << std::endl;
    std::cout << "the input formula is " << f->dump() << std::endl;

    ///@todo This belongs to the logging mechanism, and not to this file...
    //@{
//    if (automaton_dot_file != "" or epsilon_dot_file != "") {
//        spaction::automata::CltlTranslator translator(f);
//        translator.build_automaton();
//        if (automaton_dot_file != "") {
//            translator.automaton_dot(automaton_dot_file);
//            std::cerr << "automaton was printed to file " << automaton_dot_file << std::endl;
//        }
//        if (epsilon_dot_file != "") {
//            translator.epsilon_dot(epsilon_dot_file);
//            std::cerr << "epsilon-automaton was printed to file " << epsilon_dot_file << std::endl;
//        }
//    }
    //@}

    unsigned int result = spaction::find_bound_max(f, model_file, strategy);

    std::cout << "the max bound is " << result << std::endl;

    return 0;
}
