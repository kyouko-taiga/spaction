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

#include "cltl.h"

#include "atomic.h"
#include "binop.h"
#include "constant.h"
#include "unop.h"

namespace spaction {

cltl_visitor::~cltl_visitor() {}

cltl_formula::~cltl_formula() {}

cltl_formula *cltl_factory::make_atomic(const std::string &s) {
    return new atomic(s);
}

cltl_formula *
cltl_factory::make_constant(bool b) {
    return new constant(b);
}

cltl_formula *
cltl_factory::make_next(const cltl_formula *f) {
    return new unop(NEXT, f);
}

cltl_formula *
cltl_factory::make_not(const cltl_formula *f) {
    return new unop(NOT, f);
}

cltl_formula *
cltl_factory::make_and(const cltl_formula *l, const cltl_formula *r) {
    return new binop(AND, l, r);
}

cltl_formula *
cltl_factory::make_or(const cltl_formula *l, const cltl_formula *r) {
    return new binop(OR, l, r);
}

cltl_formula *
cltl_factory::make_until(const cltl_formula *l, const cltl_formula *r) {
    return new binop(UNTIL, l, r);
}

cltl_formula *
cltl_factory::make_release(const cltl_formula *l, const cltl_formula *r) {
    return new binop(RELEASE, l, r);
}

cltl_formula *
cltl_factory::make_costuntil(const cltl_formula *l, const cltl_formula *r) {
    return new binop(COST_UNTIL, l, r);
}

cltl_formula *
cltl_factory::make_costrelease(const cltl_formula *l, const cltl_formula *r) {
    return new binop(COST_RELEASE, l, r);
}

cltl_formula *
cltl_factory::make_imply(const spaction::cltl_formula *l, const spaction::cltl_formula *r) {
    cltl_formula *lhs = make_not(l);
    cltl_formula *res = make_or(lhs, r);
    lhs->destroy();
    return res;
}

cltl_formula *
cltl_factory::make_finally(const cltl_formula *f) {
    cltl_formula *ftrue = make_constant(true);
    cltl_formula *result = make_until(ftrue, f);
    ftrue->destroy();
    return result;
}

cltl_formula *
cltl_factory::make_globally(const cltl_formula *f) {
    cltl_formula *ffalse = make_constant(false);
    cltl_formula *result = make_release(ffalse, f);
    ffalse->destroy();
    return result;
}

}  // namespace spaction
