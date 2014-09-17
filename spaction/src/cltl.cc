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

#include <functional>

#include "cltl.h"

#include "atomic.h"
#include "binop.h"
#include "constant.h"
#include "unop.h"

namespace spaction {

cltl_formula_ptr cltl_factory::_make_shared_formula(cltl_formula* formula) {
    // try to find the formula within the unique index and return its shared pointer
    for (auto f : _formulae) {
        if (*f == *formula) return f->shared_from_this();
    }

    // insert the new formula in the unique index and creates its shared pointer
    _formulae.insert(formula);
    return cltl_formula_ptr(formula, std::bind(&cltl_factory::_deleter, this,
                                               std::placeholders::_1));
}

cltl_formula_ptr cltl_factory::make_atomic(const std::string &s) {
    return _make_shared_formula(new atomic(s));
}

cltl_formula_ptr cltl_factory::make_constant(bool b) {
    return _make_shared_formula(new constant(b));
}

cltl_formula_ptr cltl_factory::make_next(const cltl_formula_ptr &f) {
    return _make_shared_formula(new unop(NEXT, f));
}

cltl_formula_ptr cltl_factory::make_not(const cltl_formula_ptr &f) {
    return _make_shared_formula(new unop(NOT, f));
}

cltl_formula_ptr cltl_factory::make_and(const cltl_formula_ptr &l, const cltl_formula_ptr &r) {
    return _make_shared_formula(new binop(AND, l, r));
}

cltl_formula_ptr cltl_factory::make_or(const cltl_formula_ptr &l, const cltl_formula_ptr &r) {
    return _make_shared_formula(new binop(OR, l, r));
}

cltl_formula_ptr cltl_factory::make_until(const cltl_formula_ptr &l, const cltl_formula_ptr &r) {
    return _make_shared_formula(new binop(UNTIL, l, r));
}

cltl_formula_ptr cltl_factory::make_release(const cltl_formula_ptr &l, const cltl_formula_ptr &r) {
    return _make_shared_formula(new binop(RELEASE, l, r));
}

cltl_formula_ptr cltl_factory::make_costuntil(const cltl_formula_ptr &l,
                                              const cltl_formula_ptr &r) {
    return _make_shared_formula(new binop(COST_UNTIL, l, r));
}

cltl_formula_ptr cltl_factory::make_costrelease(const cltl_formula_ptr &l,
                                                const cltl_formula_ptr &r) {
    return _make_shared_formula(new binop(COST_RELEASE, l, r));
}

cltl_formula_ptr cltl_factory::make_imply(const cltl_formula_ptr &l, const cltl_formula_ptr &r) {
    cltl_formula_ptr lhs = make_not(l);
    cltl_formula_ptr res = make_or(lhs, r);
    return res;
}

cltl_formula_ptr cltl_factory::make_finally(const cltl_formula_ptr &f) {
    cltl_formula_ptr ftrue = make_constant(true);
    cltl_formula_ptr result = make_until(ftrue, f);
    return result;
}

cltl_formula_ptr cltl_factory::make_globally(const cltl_formula_ptr &f) {
    cltl_formula_ptr ffalse = make_constant(false);
    cltl_formula_ptr result = make_release(ffalse, f);
    return result;
}

}  // namespace spaction
