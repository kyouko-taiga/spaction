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

#include "spotcheck.h"

#include <fstream>

#include <iface/dve2/dve2.hh>
#include <ltlparse/public.hh>
#include <tgba/tgbaproduct.hh>
#include <tgbaalgos/dotty.hh>
#include <tgbaalgos/emptiness.hh>
#include <tgbaalgos/translate.hh>

#include "unop.h"
#include "visitor.h"

//#define trace std::cerr
#define trace while (0) std::cerr

namespace spaction {

bool spot_check(const std::string &ltl_string, const std::string &modelfile) {
    // spot parsing of the instantiated formula
    spot::ltl::parse_error_list pel;
    const spot::ltl::formula *ltl_formula = spot::ltl::parse(ltl_string, pel);
    if (spot::ltl::format_parse_errors(std::cerr, ltl_string, pel)) {
        ltl_formula->destroy();
        exit(1);
    }

    trace << "spot parsing done" << std::endl;

    // to store atomic propositions appearing in the formula
    spot::ltl::atomic_prop_set atomic_propositions;
    // bdd dictionnary
    spot::bdd_dict *bdd_dictionnary = new spot::bdd_dict();
    const spot::tgba *property_automaton;
    // NB: embedding the translation in a block is mandatory for proper deallocation
    {
        // translate the formula into an automaton
        spot::translator formula_translator(bdd_dictionnary);
        property_automaton = formula_translator.run(&ltl_formula);
    }

    trace << "tgba built" << std::endl;

    // collect atomic propositions from formula
    atomic_prop_collect(ltl_formula, &atomic_propositions);

    trace << "ap collected" << std::endl;

    // load divine model
    spot::kripke *model = spot::load_dve2(modelfile, bdd_dictionnary, &atomic_propositions);

    trace << "dve loaded" << std::endl;

    // synchronized product of both automata
    spot::tgba *product = new spot::tgba_product(model, property_automaton);

    trace << "product built" << std::endl;

    // emptiness check of the product automaton
    const char* echeck_algo = "Cou99";
    const char* err;
    spot::emptiness_check_instantiator* echeck_inst = nullptr;
    echeck_inst = spot::emptiness_check_instantiator::construct(echeck_algo, &err);
    // \todo properly catch incorrect instantiation of emptiness checker
    assert(echeck_inst);

    // the real emptiness check
    spot::emptiness_check* emptiness_checker = echeck_inst->instantiate(product);
    // \todo properly catch incorrect instantation
    assert(emptiness_checker);

    trace << "emptichecker built" << std::endl;

    spot::emptiness_check_result *result = nullptr;
    try {
        result = emptiness_checker->check();
    } catch (std::bad_alloc) {
        std::cerr << "out of memory during emptiness check" << std::endl;
        assert(false);
    }

    trace << "all done, prepare for delete" << std::endl;

    // free all the stuff
    delete result;
    trace << "result deleted" << std::endl;
    delete product;
    trace << "product deleted" << std::endl;
    delete model;
    trace << "model deleted" << std::endl;
    delete property_automaton;
    trace << "tgba deleted" << std::endl;
    delete bdd_dictionnary;
    trace << "bdd dict deleted" << std::endl;
    ltl_formula->destroy();
    trace << "formula deleted" << std::endl;

    trace << "all freed, return" << std::endl;

    trace << "result is " << !result << std::endl;

    trace << "done block" << std::endl;
    // output automata in .dot
//    if (filename != "") {
//        std::ofstream out(filename + ".dot");
//        spot::dotty_reachable(out, tgba, false);
//        out.close();
//    }

    // return the result
    return !result;
}

static bool spot_check_inf(const cltl_formula *formula, int n, const std::string &modelname) {
    // instantiate the cost formula
    std::string ltl_string;
    {
        cltl_formula *tmp = instantiate_inf(formula, n);
        ltl_string = tmp->dump();
        delete tmp;
    }
    return spot_check(ltl_string, modelname);
}

unsigned int find_bound_min(const cltl_formula *f, const std::string &modelname) {
    // min holds the greatest tested number for which spot_check returns true
    // max holds the smallest tested number for which spot_check returns false
    unsigned int max = 0;
    unsigned int min = 0;

    if (!spot_check_inf(f, max, modelname))
        return max;

    // increase
    do {
        // \todo safe checks to detect int overflow
        if (!max)   max = 1;
        else        max *= 2;
    } while (spot_check_inf(f, max, modelname));

    // decrease
    min = max / 2;
    while (min + 1 != max) {
        unsigned int tmp = (min + max) / 2;
        if (spot_check_inf(f, tmp, modelname))
            min = tmp;
        else
            max = tmp;
    }
    return min;
}

static bool spot_check_sup(const cltl_formula *formula, int n, const std::string &modelname) {
    std::cerr << "checking " << n << std::endl;
    // instantiate the cost formula
    std::string ltl_string;
    {
        cltl_formula *tmp = instantiate_sup(formula, n);
        cltl_formula *tmp2 = new unop(NOT, tmp);
        ltl_string = tmp2->dump();
        delete tmp2;
        delete tmp;
    }
    return spot_check(ltl_string, modelname);
}

// \todo
unsigned int find_bound_max(const cltl_formula *f, const std::string &modelname) {
    // min holds the greatest tested number for which spot_check returns true
    // max holds the smallest tested number for which spot_check returns false
    unsigned int max = 0;
    unsigned int min = 0;

    if (!spot_check_sup(f, max, modelname))
        return max;

    // increase
    do {
        // \todo safe checks to detect int overflow
        if (!max)   max = 1;
        else        max *= 2;
    } while (spot_check_sup(f, max, modelname));

    // decrease
    min = max / 2;
    while (min + 1 != max) {
        unsigned int tmp = (min + max) / 2;
        if (spot_check_sup(f, tmp, modelname))
            min = tmp;
        else
            max = tmp;
    }
    return max;
}

}  // namespace spaction
