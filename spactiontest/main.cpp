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

// this file is strongly inspired from spot/iface/dve2/dve2check.cc

int main(int argc, char* argv[]) {
    using namespace spaction;

    if (argc != 2) {
        std::cerr << "wrong number of arguments" << std::endl;
        std::cerr << "usage: spaction model.dve" << std::endl;
        return 1;
    }

    // a test formula
    // \todo atoms are not properly deleted here
    cltl_formula *f = cltl_factory::make_costuntil(cltl_factory::make_atomic("P_0.wait"),
                                                   cltl_factory::make_atomic("P_0.CS"));

    std::cout << f->dump() << std::endl;

    int bound = find_bound_min(f, argv[1]);
    std::cout << "the bound for " << f->dump() << " is " << bound << std::endl;

    f->destroy();

    return 0;
}
