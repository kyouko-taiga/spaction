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

#include "MultOperator.h"

#include "CltlFormulaFactory.h"
#include "CltlFormulaVisitor.h"
#include "hash/hash.h"

namespace spaction {

MultOperator::MultOperator(MultOperatorType type, const std::vector<CltlFormulaPtr> &childs,
                           CltlFormulaFactory *creator) :
    CltlFormula(creator), _type(type), _childs(childs) {
}

std::size_t MultOperator::hash() const {
    // @todo take type into account in the hash (to avoid collisions)
    return std::hash<std::vector<CltlFormulaPtr>>()(_childs);
}

bool MultOperator::syntactic_eq(const CltlFormula &rhs) const {
    if (rhs.formula_type() != CltlFormula::kMultOperator)
        return false;

    const MultOperator &bo = static_cast<const MultOperator &>(rhs);
    if (bo._type != _type)
        return false;

    return _childs == bo._childs;
}

std::size_t MultOperator::height() const {
    std::size_t result = 0;
    for (auto &c: _childs) {
        result = std::max(result, c->height());
    }
    return result+1;
}

bool MultOperator::is_infltl() const {
    for (auto &c: _childs) {
        if (!c->is_infltl())
            return false;
    }
    return true;
}

bool MultOperator::is_supltl() const {
    for (auto &c: _childs) {
        if (!c->is_supltl())
            return false;
    }
    return true;
}

bool MultOperator::is_propositional() const {
    for (auto &c: _childs) {
        if (!c->is_propositional())
            return false;
    }
    return true;
}

bool MultOperator::is_nnf() const {
    for (auto &c: _childs) {
        if (!c->is_nnf())
            return false;
    }
    return true;
}

void MultOperator::accept(CltlFormulaVisitor &visitor) {
    // explicitly cast shared_from_this to the a derived class shared_ptr
    visitor.visit(std::dynamic_pointer_cast<MultOperator>(shared_from_this()));
}

CltlFormulaPtr MultOperator::to_nnf() {
    std::vector<CltlFormulaPtr> tmp;
    for (auto &c: _childs) {
        tmp.push_back(c->to_nnf());
    }
    return _creator->make_nary(_type, tmp);
}

std::string MultOperator::dump() const {
    std::string result;
    for (auto it = _childs.begin() ; it != _childs.end() ; ++it) {
        result += "(";
        result += (*it)->dump();
        result += ")";
        if (it+1 != _childs.end()) {
            switch (_type) {
                case kOr:
                    result += "||";
                    break;
                case kAnd:
                    result += "&&";
                    break;
            }
        }
    }
    return result;
}

}  // namespace spaction
