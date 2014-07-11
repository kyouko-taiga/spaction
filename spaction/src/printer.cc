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

class instant : public cltl_visitor {
 public:
    explicit instant(int n): n(n), res(nullptr) {}
    ~instant() { delete res; }

    void visit(const atomic *node) final {
        res = node->clone();
    }

    void visit(const constant * node) final {
        res = node->clone();
    }

    void visit(const unop *node) final {
        cltl_formula * tmp = instantiate(node->sub(), n);
        res = new unop(node->get_type(), tmp);
        delete tmp;
    }

    void visit(const binop *node) final {
        cltl_formula *l = instantiate(node->left(), n);
        cltl_formula *r = instantiate(node->right(), n);

        cltl_formula *ltmp = nullptr;
        cltl_formula *rtmp = nullptr;
        cltl_formula *ltmp2 = nullptr;
        cltl_formula *rtmp2 = nullptr;
        cltl_formula *tmp2 = nullptr;
        cltl_formula *tmp3 = nullptr;

        switch (node->get_type()) {
            case AND:
            case OR:
            case UNTIL:
            case RELEASE:
                res = new binop(node->get_type(), l, r);
                break;
            case COST_UNTIL:
                if (n == 0) {
                    res = new binop(UNTIL, l, r);
                } else {
                    ltmp = new binop(UNTIL, l, r);
                    ltmp2 = new unop(NOT, l);
                    tmp3 = instantiate(node, n-1);
                    rtmp2 = new unop(NEXT, tmp3);
                    tmp2 = new binop(AND, ltmp2, rtmp2);
                    rtmp = new binop(UNTIL, l, tmp2);
                    res = new binop(OR, ltmp, rtmp);
                    delete rtmp;
                    delete tmp2;
                    delete rtmp2;
                    delete tmp3;
                    delete ltmp2;
                    delete ltmp;
                }
                break;
            case COST_RELEASE:
                // \todo but should not happen here
                std::cerr << "we should not encounter a cost release during instantiation";
                std::cerr << std::endl;
                exit(1);
                break;
        }

        delete l;
        delete r;
    }

    cltl_formula *get_result() const {
        return res->clone();
    }

 private:
    int n;
    cltl_formula *res;
};

cltl_formula *instantiate(const cltl_formula *formula, int n) {
    // n > 0
    instant visitor(n);
    formula->accept(visitor);
    cltl_formula * res = visitor.get_result();
    return res;
}

}  // namespace spaction
