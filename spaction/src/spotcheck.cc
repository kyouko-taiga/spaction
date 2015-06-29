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

#include <iface/ltsmin/ltsmin.hh>
#include <ltlparse/public.hh>
#include <twa/twaproduct.hh>
#include <twaalgos/dot.hh>
#include <twaalgos/emptiness.hh>
#include <twaalgos/stats.hh>
#include <twaalgos/translate.hh>

#include "AtomicProposition.h"
#include "CltlFormulaFactory.h"
#include "automata/CltlTranslator.h"
#include "automata/CounterAutomatonProduct.h"
#include "automata/SupremumFinder.h"
#include "automata/TGBA2CA.h"

#include "Instantiator.h"
#include "automata/CA2tgba.h"

#include "Logger.h"

namespace spaction {

bool spot_dve_check(const std::string &formula, const std::string &modelfile) {
    // spot parsing of the instantiated formula
    const spot::ltl::formula *ltl_formula = spot::ltl::parse_formula(formula);

    LOG_INFO << "spot parsing done" << std::endl;

    // to store atomic propositions appearing in the formula
    spot::ltl::atomic_prop_set atomic_propositions;
    // bdd dictionnary
    spot::bdd_dict_ptr bdd_dictionnary = spot::make_bdd_dict();
    spot::const_twa_ptr property_automaton = nullptr;
    // NB: embedding the translation in a block is mandatory for proper deallocation
    {
        // translate the formula into an automaton
        spot::translator formula_translator(bdd_dictionnary);
        property_automaton = formula_translator.run(&ltl_formula);
    }

    LOG_INFO << "property tgba built" << std::endl;

    // collect atomic propositions from formula
    atomic_prop_collect(ltl_formula, &atomic_propositions);

    LOG_INFO << "ap collected" << std::endl;

    // load divine model
    spot::kripke_ptr model = spot::load_ltsmin(modelfile, bdd_dictionnary, &atomic_propositions);

    LOG_INFO << "dve loaded" << std::endl;

    // synchronized product of both automata
    spot::twa_ptr product = spot::otf_product(model, property_automaton);

    LOG_INFO << "product automaton built" << std::endl;

    // emptiness check of the product automaton
    spot::emptiness_check_instantiator_ptr echeck_inst = nullptr;
    {  // build the instantiator
        // @todo add an option to select what EC to use
        const char* echeck_algo = "Cou99";
        const char* err;
        echeck_inst = spot::make_emptiness_check_instantiator(echeck_algo, &err);
        // check correct instantiation.
        // according to spot documentation, `make_emptiness_check_instantiator` returns 0 on failure,
        // and an error log in err.
        if (!echeck_inst) {
            LOG_FATAL << "Emptiness Check Instantiator could not be built: " << err << " is not recognized" << std::endl;
            throw std::runtime_error("Fail to build Emptiness Check Instantiator");
        }
    }

    // the real emptiness check
    spot::emptiness_check_ptr emptiness_checker = echeck_inst->instantiate(product);
    if (!emptiness_checker) {
        LOG_FATAL << "Emptiness checker could not be built" << std::endl;
        throw std::runtime_error("Fail to build Emptiness checker");
    }

    LOG_INFO << "emptichecker built" << std::endl;

    spot::emptiness_check_result_ptr result = nullptr;
    try {
        result = emptiness_checker->check();
    } catch (std::bad_alloc) {
        LOG_FATAL << "out of memory during emptiness check" << std::endl;
        throw std::bad_alloc();
    }
    bool to_return = !result;
    LOG_INFO << "result of emptiness check is " << to_return << std::endl;

    // free all the stuff (most are smart pointers)
    ltl_formula->destroy();

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
                                 spot::kripke_ptr model,
                                 spot::bdd_dict_ptr dict) {
    assert(formula->is_supltl());
    // sup \emptyset = 0
    automata::value_t res = {false, 0};
    CltlFormulaPtr phi = formula;

    // the emptiness check instantiator
    spot::emptiness_check_instantiator_ptr echeck_inst = nullptr;

    {  // build the instantiator
        // @todo add an option to select what EC to use
        const char* echeck_algo = "Cou99";
        const char* err;
        echeck_inst = spot::make_emptiness_check_instantiator(echeck_algo, &err);
        // check correct instantiation.
        // according to spot documentation, `make_emptiness_check_instantiator` returns 0 on failure,
        // and an error log in err.
        if (!echeck_inst) {
            LOG_FATAL << "Emptiness Check Instantiator could not be built: " << err << " is not recognized" << std::endl;
            throw std::runtime_error("Fail to build Emptiness Check Instantiator");
        }
    }

    LOG_INFO << "Emptiness Check Instantiator built" << std::endl;

    // see the model as a counter automaton
    assert(model);
    automata::tgba_ca *model_ca = new automata::tgba_ca(model);

    LOG_INFO << "model loaded as a CA" << std::endl;

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

        LOG_INFO << "formula translated to CA" << std::endl;

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

        LOG_INFO << "formula translated to automaton" << std::endl;

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

        LOG_INFO << "product done" << std::endl;

        auto prod_tgba = automata::make_tgba(&prod);

        LOG_INFO << "product as tgba" << std::endl;

        // the real emptiness check
        spot::emptiness_check_ptr emptiness_checker = echeck_inst->instantiate(prod_tgba);
        if (!emptiness_checker) {
            LOG_FATAL << "Emptiness checker could not be built" << std::endl;
            throw std::runtime_error("Fail to build Emptiness checker");
        }

        LOG_INFO << "emptichecker built" << std::endl;

        spot::emptiness_check_result_ptr result = nullptr;
        try {
            result = emptiness_checker->check();
        } catch (std::bad_alloc) {
            LOG_FATAL << "out of memory during emptiness check" << std::endl;
            throw std::bad_alloc();
        }

        LOG_INFO << i << "th iteration, EC done" << std::endl;

        if ((is_nonempty = (result != nullptr))) {
            LOG_INFO << i << "th iteration, CE exists" << std::endl;

            // @note run is a run of prod_tgba
            spot::tgba_run_ptr run = result->accepting_run();
            if (!run) {
                LOG_FATAL << "The emptiness check algo used cannot compute a counterexample" << std::endl;
                throw std::runtime_error("Fail to compute a counterexample");
            }

            LOG_INFO << i << "th iteration, CE found" << std::endl;

            auto value = prod_tgba->value_word(run, upper_bound, formula->creator());
            if (value.unbounded_max) {  // infty
                res.value = 0;
                res.infinite = true;
                is_nonempty = false;
            } else {
                LOG_DEBUG << "n is " << res.value << " whereas value.max is " << value.max << std::endl;
                assert(first_pass or res.value < value.max);
                res.value = value.max;
                CltlFormulaPtr phin = (*instantiator)(formula, res.value+1);
                phi = formula->creator()->make_and(formula, phin);
            }

            LOG_INFO << i << "th iteration, n is now " << res.value << std::endl;
            LOG_INFO << "and phi is now " << phi->dump() << std::endl;
        }

        first_pass = false;
    } while (is_nonempty);

    delete instantiator;
    delete model_ca;

    return res;
}

// @param   formula is assumed to be CLTL[>]
automata::value_t find_max_direct(const CltlFormulaPtr &formula,
                                  spot::kripke_ptr model,
                                  spot::bdd_dict_ptr dict) {
    assert(formula->is_supltl());
    assert(model);
    automata::tgba_ca *model_ca = new automata::tgba_ca(model);

    LOG_INFO << "model loaded as a CA" << std::endl;

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
        LOG_INFO << "visiting AP " << formula->value() << std::endl;
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
    spot::bdd_dict_ptr bdd_dictionnary = spot::make_bdd_dict();
    auto visitor = APCollector();
    formula->accept(visitor);

    LOG_INFO << "atomic propositions collected" << std::endl;

    spot::ltl::atomic_prop_set atomic_propositions = visitor.get();

    // load divine model
    spot::kripke_ptr model = spot::load_ltsmin(modelname, bdd_dictionnary, &atomic_propositions);

    LOG_INFO << "Kripke model loaded" << std::endl;

    automata::value_t result;
    switch (strat) {
        case BoundSearchStrategy::CEGAR:
            result = find_max_cegar(formula, model, bdd_dictionnary);
            break;
        case BoundSearchStrategy::DIRECT:
            result = find_max_direct(formula, model, bdd_dictionnary);
            break;
    }

    if (result.infinite)
        return -1;
    else
        return result.value;
}

/// a helper function that loads a LTL formula as a counterless CA (TGBA seen as a CA)
automata::tgba_ca *load_formula(const std::string &formula) {
    // spot parsing of the instantiated formula
    const spot::ltl::formula *ltl_formula = spot::ltl::parse_formula(formula);

    LOG_INFO << "spot parsing done" << std::endl;

    // to store atomic propositions appearing in the formula
    spot::ltl::atomic_prop_set atomic_propositions;
    // bdd dictionnary
    spot::bdd_dict_ptr bdd_dictionnary = spot::make_bdd_dict();
    spot::const_twa_ptr property_automaton = nullptr;
    // NB: embedding the translation in a block is mandatory for proper deallocation
    {
        // translate the formula into an automaton
        spot::translator formula_translator(bdd_dictionnary);
        property_automaton = formula_translator.run(&ltl_formula);
    }

    LOG_INFO << "property tgba built" << std::endl;

    return new automata::tgba_ca(property_automaton);
}

}  // namespace spaction
