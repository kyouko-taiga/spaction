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
#include <sstream>

#include <iface/dve2/dve2.hh>
#include <ltlparse/public.hh>
#include <tgba/tgbaproduct.hh>
#include <tgbaalgos/dotty.hh>
#include <tgbaalgos/emptiness.hh>
#include <tgbaalgos/stats.hh>
#include <tgbaalgos/translate.hh>

#include "AtomicProposition.h"
#include "CltlFormulaFactory.h"
#include "automata/CltlTranslator.h"
#include "automata/CounterAutomatonProduct.h"
#include "automata/SupremumFinder.h"
#include "automata/TGBA2CA.h"

#include "Instantiator.h"
#include "automata/CA2tgba.h"

// @todo convert this to a logging mechanism
// #define TRACE
#ifdef TRACE
#define trace std::cerr
#else
#define trace while (0) std::cerr
#endif

namespace spaction {

bool spot_dve_check(const std::string &formula, const std::string &modelfile) {
    // spot parsing of the instantiated formula
    spot::ltl::parse_error_list pel;
    const spot::ltl::formula *ltl_formula = spot::ltl::parse(formula, pel);
    if (spot::ltl::format_parse_errors(std::cerr, formula, pel)) {
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

    trace << "property tgba built" << std::endl;

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
    bool to_return = !result;
    trace << "result is " << to_return << std::endl;

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

    // return the result
    return to_return;
}

// @param formula is assumed to be CLTL[<=]
static bool spot_check_inf(const CltlFormulaPtr &formula, int n, const std::string &modelname) {
    assert(formula->is_infltl());
    // instantiate the cost formula
    std::string ltl_string;
    InstantiateInf instanciator;

    const CltlFormulaPtr &tmp = instanciator(formula, n);
    ltl_string = tmp->dump();
    // to please spot, replace single quotes by double quotes around atomic propostions
    std::replace(ltl_string.begin(), ltl_string.end(), '\'', '"');
    return spot_dve_check(ltl_string, modelname);
}

// @param   formula is assumed to be CLTL[<=]
// @todo    do not use 'blind' dichotomy, use bound |aut| \times |system|
unsigned int find_bound_min_dichoto(const CltlFormulaPtr &formula, const std::string &modelname) {
    assert(formula->is_infltl());
    // min holds the greatest tested number for which spot_check returns true
    // max holds the smallest tested number for which spot_check returns false
    unsigned int max = 0;
    unsigned int min = 0;

    if (!spot_check_inf(formula, max, modelname))
        return max;

    // increase
    do {
        // \todo safe checks to detect int overflow
        if (!max)   max = 1;
        else        max *= 2;
    } while (spot_check_inf(formula, max, modelname));

    // decrease
    min = max / 2;
    while (min + 1 != max) {
        unsigned int tmp = (min + max) / 2;
        if (spot_check_inf(formula, tmp, modelname))
            min = tmp;
        else
            max = tmp;
    }
    return min;
}

// @param formula is assumed to be CLTL[>]
static bool spot_check_sup(const CltlFormulaPtr &formula, int n, const std::string &modelname) {
    assert(formula->is_supltl());
    // instantiate the cost formula
    std::string ltl_string;
    InstantiateSup instanciator;

    const CltlFormulaPtr &tmp = instanciator(formula, n);
    ltl_string = tmp->dump();
    return spot_dve_check(ltl_string, modelname);
}

// @param   formula is assumed to be CLTL[>]
// @todo    a +1 is probably missing around here
// @todo    do not use 'blind' dichotomy, use bound |aut| \times |system|
unsigned int find_bound_max_dichoto(const CltlFormulaPtr &formula, const std::string &modelname) {
    assert(formula->is_supltl());
    // min holds the greatest tested number for which spot_check returns true
    // max holds the smallest tested number for which spot_check returns false
    unsigned int max = 0;
    unsigned int min = 0;

    if (spot_check_sup(formula, max, modelname))
        return max;

    // increase
    do {
        // \todo safe checks to detect int overflow
        if (!max)   max = 1;
        else        max *= 2;
    } while (!spot_check_sup(formula, max, modelname));

    // decrease
    min = max / 2;
    while (min + 1 != max) {
        unsigned int tmp = (min + max) / 2;
        if (!spot_check_sup(formula, tmp, modelname))
            min = tmp;
        else
            max = tmp;
    }
    return min;
}

// @param   formula is assumed to be CLTL[>]
automata::value_t find_max_cegar(const CltlFormulaPtr &formula,
                                 spot::kripke *model,
                                 spot::bdd_dict *dict) {
    assert(formula->is_supltl());
    // sup \emptyset = 0
    automata::value_t res = {false, 0};
    CltlFormulaPtr phi = formula;

    // the emptiness check instantiator
    spot::emptiness_check_instantiator* echeck_inst = nullptr;

    {  // build the instantiator
        // @todo add an option to select what EC to use
        const char* echeck_algo = "Cou99";
        const char* err;
        echeck_inst = spot::emptiness_check_instantiator::construct(echeck_algo, &err);
        // @todo properly catch incorrect instantiation of emptiness checker
        assert(echeck_inst);
    }

    trace << "EC instantiator built" << std::endl;

    // see the model as a counter automaton
    assert(model);
    automata::tgba_ca *model_ca = new automata::tgba_ca(model);

    trace << "model loaded as a CA" << std::endl;

    // determine the bound to use (|model| \times |automaton of the formula|)
    //@{
    // compute model size (number of nodes)
    unsigned int model_size = spot::stats_reachable(model).states;
    // compute formula automaton size (number of nodes)
    unsigned int formula_aut_size = 0;
    {
        // turn the formula into a counter automaton
        automata::CltlTranslator translator(formula);
        translator.build_automaton();

        trace << "formula translated to CA" << std::endl;

        for (auto state : translator.get_automaton().transition_system()->states()) {
            ++formula_aut_size;
        }
    }
    unsigned int upper_bound = model_size * formula_aut_size;
    //@}

    // the CLTL2LTL instantiator
    Instantiator *instantiator = new InstantiateSup();

    bool is_nonempty = false;
    int i = 0;  // counts the number of runs of the loop
    bool first_pass = true;
    do {
        i++;
        // build the automaton of phi
        automata::CltlTranslator translator(phi);
        translator.build_automaton();

        trace << "formula translated to automaton" << std::endl;

// @todo merge to logging mechanism
#ifdef TRACE
        std::stringstream ca_file;
        ca_file << "ca_" << i << ".dot";
        translator.get_automaton().print(ca_file.str());
#endif

        auto prod = automata::make_aut_product(translator.get_automaton(), *model_ca, dict, formula->creator());

// @todo merge to logging mechanism
#ifdef TRACE
        std::stringstream prod_file;
        prod_file << "prod_" << i << ".dot";
        prod.print(prod_file.str());
#endif

        trace << "product done" << std::endl;

        auto prod_tgba = automata::make_tgba(&prod);

        trace << "product as tgba" << std::endl;

        // the real emptiness check
        spot::emptiness_check* emptiness_checker = echeck_inst->instantiate(prod_tgba);
        // @todo properly catch incorrect instantation
        assert(emptiness_checker);

        trace << "emptichecker built" << std::endl;

        spot::emptiness_check_result *result = nullptr;
        try {
            result = emptiness_checker->check();
        } catch (std::bad_alloc) {
            std::cerr << "out of memory during emptiness check" << std::endl;
            assert(false);
        }

        trace << i << "th iteration, EC done" << std::endl;

        if ((is_nonempty = result)) {
            trace << i << "th iteration, CE exists" << std::endl;

            // @note run is a run of prod_tgba
            spot::tgba_run *run = result->accepting_run();
            assert(run);

            trace << i << "th iteration, CE found" << std::endl;

            auto value = prod_tgba->value_word(run, upper_bound, formula->creator());
            if (value.unbounded_max) {  // infty
                res.value = 0;
                res.infinite = true;
                is_nonempty = false;
            } else {
                trace << "n is " << res.value << " whereas value.max is " << value.max << std::endl;
                assert(first_pass or res.value < value.max);
                res.value = value.max;
                CltlFormulaPtr phin = (*instantiator)(formula, res.value+1);
                phi = formula->creator()->make_and(formula, phin);
            }
            delete run;

            trace << i << "th iteration, n is now " << res.value << std::endl;
            trace << "and phi is now " << phi->dump() << std::endl;
        }

        first_pass = false;

        delete result;
        delete emptiness_checker;
        delete prod_tgba;
    } while (is_nonempty);

    delete instantiator;
    delete model_ca;

    return res;
}

// @param   formula is assumed to be CLTL[>]
automata::value_t find_max_direct(const CltlFormulaPtr &formula,
                                  spot::kripke *model,
                                  spot::bdd_dict *dict) {
    assert(formula->is_supltl());
    assert(model);
    automata::tgba_ca *model_ca = new automata::tgba_ca(model);

    trace << "model loaded as a CA" << std::endl;

    automata::CltlTranslator translator(formula);
    translator.build_automaton();

    auto prod = automata::make_aut_product(translator.get_automaton(), *model_ca, dict, formula->creator());

    auto config_aut = automata::make_minmax_configuration_automaton(prod);
    auto sup_comput = automata::make_sup_comput(config_aut);

    // compute model size (number of nodes)
    unsigned int model_size = spot::stats_reachable(model).states;
    // compute formula automaton size (number of nodes)
    unsigned int formula_aut_size = 0;
    for (auto state : translator.get_automaton().transition_system()->states()) {
        ++formula_aut_size;
    }

    return sup_comput.find_supremum(model_size * formula_aut_size);
}

/// A CLTL formula visitor to collect the AP used by a formula.
class APCollector : public CltlFormulaVisitor {
 public:
    virtual ~APCollector() { }

    void visit(const std::shared_ptr<AtomicProposition> &formula) final {
        trace << "visiting AP " << formula->value() << std::endl;
        _res.insert(spot::ltl::atomic_prop::instance(formula->value(), spot::ltl::default_environment::instance()));
    }

    void visit(const std::shared_ptr<ConstantExpression> &formula) final {}
    void visit(const std::shared_ptr<UnaryOperator> &formula) final {
        formula->operand()->accept(*this);
    }
    void visit(const std::shared_ptr<BinaryOperator> &formula) final {
        formula->left()->accept(*this);
        formula->right()->accept(*this);
    }

    spot::ltl::atomic_prop_set get() const { return _res; }

 private:
    spot::ltl::atomic_prop_set _res;
};

// @param   formula is assumed to be CLTL[>]
// @param   modelname is the path to a .dve model
unsigned int find_bound_max(const CltlFormulaPtr &formula, const std::string &modelname,
                            BoundSearchStrategy strat) {
    assert(formula->is_supltl());
    // get atomic propositions from formula
    spot::bdd_dict bdd_dictionnary;
    auto visitor = APCollector();
    formula->accept(visitor);

    trace << "ap collected" << std::endl;

    spot::ltl::atomic_prop_set atomic_propositions = visitor.get();

    // load divine model
    spot::kripke *model = spot::load_dve2(modelname, &bdd_dictionnary, &atomic_propositions);

    trace << "model loaded" << std::endl;

    automata::value_t result;
    switch (strat) {
        case BoundSearchStrategy::CEGAR:
            result = find_max_cegar(formula, model, &bdd_dictionnary);
            break;
        case BoundSearchStrategy::DIRECT:
            result = find_max_direct(formula, model, &bdd_dictionnary);
            break;
    }

    // delete the model
    delete model;
    if (result.infinite)
        return -1;
    else
        return result.value;
}

/// a helper function that loads a LTL formula as a counterless CA (TGBA seen as a CA)
automata::tgba_ca *load_formula(const std::string &formula) {
    // spot parsing of the instantiated formula
    spot::ltl::parse_error_list pel;
    const spot::ltl::formula *ltl_formula = spot::ltl::parse(formula, pel);
    if (spot::ltl::format_parse_errors(std::cerr, formula, pel)) {
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

    trace << "property tgba built" << std::endl;

    return new automata::tgba_ca(property_automaton);
}

}  // namespace spaction
