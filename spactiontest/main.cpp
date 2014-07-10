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

#include "cltl.h"

#include "atomic.h"
#include "constant.h"
#include "binop.h"
#include "unop.h"

#include "spotcheck.h"

#include <iface/dve2/dve2.hh>

// this file is strongly inspired from spot/iface/dve2/dve2check.cc

int main(int argc, char** argv) {
    int exit_code = 0;

    cltl_formula * f = new binop (COST_UNTIL,
                                  new atomic ("wait"),
                                  new atomic ("CS"));

    std::cout << f->dump () << std::endl;

    spot::ltl::atomic_prop_set ap;
    spot::bdd_dict * dict = new spot::bdd_dict();
    spot::kripke * model = nullptr;
    // model = spot::load_dve2(argv[1], dict, &ap/*, deadf, compress_states, true*/);

    if (!model)
    {
        exit_code = 1;
        goto safe_exit;
    }

    spot_check (f, 0, "g0");
    spot_check (f, 1, "g1");
    spot_check (f, 2, "g2");

    //  cltl_formula * g0 = instantiate (f, 0);
    //  std::cout << g0->dump () << std::endl;
    //  cltl_formula * g1 = instantiate (f, 1);
    //  std::cout << g1->dump () << std::endl;
    //  cltl_formula * g2 = instantiate (f, 2);
    //  std::cout << g2->dump () << std::endl;

safe_exit:
    delete model;
    delete dict;
    delete f;
    
    return exit_code;
}
