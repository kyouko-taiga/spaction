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

#include "visitor.h"
#include "atomic.h"
#include "constant.h"
#include "binop.h"
#include "unop.h"

namespace spaction {

class instant_inf : public cltl_visitor {
 public:
    explicit instant_inf(int n): n(n), res(nullptr) {}
    ~instant_inf() { res->destroy(); }

    void visit(const atomic *node) final {
        res = node->clone();
    }

    void visit(const constant * node) final {
        res = node->clone();
    }

    void visit(const unop *node) final {
        cltl_formula *sub_instantiated = instantiate_inf(node->sub(), n);
        switch (node->get_type()) {
            case NEXT:
                res = cltl_factory::make_next(sub_instantiated);
                break;
            case NOT:
                res = cltl_factory::make_not(sub_instantiated);
                break;
        }
        sub_instantiated->destroy();
    }

    void visit(const binop *node) final {
        cltl_formula *l = instantiate_inf(node->left(), n);
        cltl_formula *r = instantiate_inf(node->right(), n);

        cltl_formula *ltmp = nullptr;
        cltl_formula *rtmp = nullptr;
        cltl_formula *ltmp2 = nullptr;
        cltl_formula *rtmp2 = nullptr;
        cltl_formula *tmp2 = nullptr;
        cltl_formula *tmp3 = nullptr;

        switch (node->get_type()) {
            case AND:
                res = cltl_factory::make_and(l, r);
                break;
            case OR:
                res = cltl_factory::make_or(l, r);
                break;
            case UNTIL:
                res = cltl_factory::make_until(l, r);
                break;
            case RELEASE:
                res = cltl_factory::make_release(l, r);
                break;
            case COST_UNTIL:
                // a U{n=0} b -> a U b
                // a U{n}   b -> (a U b) || (a U ((!a) && (X (a U{n-1} b))))
                if (n == 0) {
                    res = cltl_factory::make_until(l, r);
                } else {
                    ltmp = cltl_factory::make_until(l, r);
                    ltmp2 = cltl_factory::make_not(l);
                    tmp3 = instantiate_inf(node, n-1);
                    rtmp2 = cltl_factory::make_next(tmp3);
                    tmp2 = cltl_factory::make_and(ltmp2, rtmp2);
                    rtmp = cltl_factory::make_until(l, tmp2);
                    res = cltl_factory::make_or(ltmp, rtmp);
                    rtmp->destroy();
                    tmp2->destroy();
                    rtmp2->destroy();
                    tmp3->destroy();
                    ltmp2->destroy();
                    ltmp->destroy();
                }
                break;
            case COST_RELEASE:
                // \todo but should not happen here
                std::cerr << "we should not encounter a cost release during inf instantiation";
                std::cerr << std::endl;
                exit(1);
                break;
        }

        l->destroy();
        r->destroy();
    }

    cltl_formula *get_result() const {
        return res->clone();
    }

 private:
    int n;
    cltl_formula *res;
};

cltl_formula *instantiate_inf(const cltl_formula *formula, int n) {
    instant_inf visitor(n);
    formula->accept(visitor);
    cltl_formula * res = visitor.get_result();
    return res;
}

class instant_sup : public cltl_visitor {
public:
    explicit instant_sup(int n): n(n), res(nullptr) {}
    ~instant_sup() { res->destroy(); }

    void visit(const atomic *node) final {
        res = node->clone();
    }

    void visit(const constant * node) final {
        res = node->clone();
    }

    void visit(const unop *node) final {
        cltl_formula *sub_instantiated = instantiate_sup(node->sub(), n);
        switch (node->get_type()) {
            case NEXT:
                res = cltl_factory::make_next(sub_instantiated);
                break;
            case NOT:
                res = cltl_factory::make_not(sub_instantiated);
                break;
        }
        sub_instantiated->destroy();
    }

    void visit(const binop *node) final {
        cltl_formula *l = instantiate_sup(node->left(), n);
        cltl_formula *r = instantiate_sup(node->right(), n);

//        cltl_formula *ltmp = nullptr;
//        cltl_formula *rtmp = nullptr;
        cltl_formula *ltmp2 = nullptr;
        cltl_formula *rtmp2 = nullptr;
        cltl_formula *tmp2 = nullptr;
        cltl_formula *tmp3 = nullptr;

        switch (node->get_type()) {
            case AND:
                res = cltl_factory::make_and(l, r);
                break;
            case OR:
                res = cltl_factory::make_or(l, r);
                break;
            case UNTIL:
                res = cltl_factory::make_until(l, r);
                break;
            case RELEASE:
                res = cltl_factory::make_release(l, r);
                break;
            case COST_UNTIL:
                // a U{n=0} b -> true U b
                // a U{n}   b -> a U (!a && X (a U{n-1} b))
                if (n == 0) {
                    cltl_formula *ctrue = cltl_factory::make_constant(true);
                    res = cltl_factory::make_until(ctrue, r);
                    ctrue->destroy();
                } else {
                    tmp3 = instantiate_sup(node, n-1);
                    ltmp2 = cltl_factory::make_not(l);
                    rtmp2 = cltl_factory::make_next(tmp3);
                    tmp2 = cltl_factory::make_until(ltmp2, rtmp2);
                    res = cltl_factory::make_until(l, tmp2);
                    tmp3->destroy();
                    tmp2->destroy();
                    rtmp2->destroy();
                    ltmp2->destroy();
                }
                break;
            case COST_RELEASE:
                // \todo but should not happen here
                std::cerr << "we should not encounter a cost release during inf instantiation";
                std::cerr << std::endl;
                exit(1);
                break;
        }
        
        l->destroy();
        r->destroy();
    }
    
    cltl_formula *get_result() const {
        return res->clone();
    }
    
private:
    int n;
    cltl_formula *res;
};

cltl_formula *instantiate_sup(const cltl_formula *formula, int n) {
    instant_sup visitor(n);
    formula->accept(visitor);
    cltl_formula * res = visitor.get_result();
    return res;
}

}  // namespace spaction
