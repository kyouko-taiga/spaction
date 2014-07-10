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

#include <fstream>

#include <ltlparse/public.hh>
#include <tgbaalgos/ltl2tgba_fm.hh>
#include <tgbaalgos/sccfilter.hh>
#include <tgbaalgos/simulation.hh>
#include <tgbaalgos/dotty.hh>

#include "spotcheck.h"
#include "visitor.h"

bool spot_check(const cltl_formula *formula, int n, const std::string &filename) {
    std::string ltl_string;
    {
        cltl_formula *tmp = instantiate(formula, n);
        ltl_string = tmp->dump();
        delete tmp;
    }

    spot::ltl::parse_error_list pel;

    const spot::ltl::formula *ltl_formula = spot::ltl::parse(ltl_string, pel);
    if (spot::ltl::format_parse_errors(std::cerr, ltl_string, pel)) {
        ltl_formula->destroy();
        exit(1);
    }

    // simplify formula
    spot::ltl::ltl_simplifier_options simplify_opt;
    spot::ltl::ltl_simplifier simplifier(simplify_opt);
    ltl_formula = simplifier.simplify(ltl_formula);

    // bdd dictionnary
    spot::bdd_dict bdddict;

    // build the automaton
    const spot::tgba *tgba = spot::ltl_to_tgba_fm(ltl_formula, &bdddict);

    // simplify the automaton
    {
        // use scc_filter_states for sba, and scc_filter for tgba
        const spot::tgba *tt = spot::scc_filter(tgba);
        delete tgba;
        tgba = tt;
        tt = iterated_simulations(tgba);
        delete tgba;
        tgba = tt;
    }

    // output formula in text
    std::cerr << "formula is " << formula->dump() << std::endl;

    // output automata in .dot
    if (filename != "") {
        std::ofstream out(filename + ".dot");
        spot::dotty_reachable(out, tgba, false);
        out.close();
    }

    // \todo check against the model

    // free tgba
    delete tgba;
    // free formula
    ltl_formula->destroy();

    // \todo return the result
    return true;
}

int find_bound(const cltl_formula * f) {
    // \todo safe checks to detect int overflow
    int res = 0;

    // increase
    do {
        if (!res) res = 1;
        else      res *= 2;
    } while (spot_check(f, res));

    // decrease
    do {
        // \todo a dichotomic search could also be done here
        res--;
    } while (!spot_check(f, res));
    return res;
}
