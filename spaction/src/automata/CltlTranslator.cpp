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

#include "automata/CltlTranslator.h"

#include <algorithm>

#include "BinaryOperator.h"
#include "CltlFormulaFactory.h"
#include "ConstantExpression.h"
#include "UnaryOperator.h"

namespace spaction {
namespace automata {

CltlTranslator::CltlTranslator(const CltlFormulaPtr &formula) :
    _formula(formula->to_nnf()), _nb_counters(0) {
        _formula->accept(*this);
}

void CltlTranslator::build_automaton() {
    _build_transition_system();
}

void CltlTranslator::visit(const std::shared_ptr<UnaryOperator> &formula) {
    formula->operand()->accept(*this);
}

void CltlTranslator::visit(const std::shared_ptr<BinaryOperator> &formula) {
    switch (formula->operator_type()) {
        case BinaryOperator::kCostUntil:
        case BinaryOperator::kCostRelease:
            _counters_maps[formula] = _nb_counters++;
            break;
        default:
            break;
    }
    formula->left()->accept(*this);
    formula->right()->accept(*this);
}

CltlTranslator::FormulaList CltlTranslator::_unique_sort(const CltlTranslator::FormulaList &terms) {
    FormulaList result(terms);
    // sort
    std::sort(result.begin(), result.end(),
              [](const CltlFormulaPtr &l, const CltlFormulaPtr &r){
                  return (l->height() == r->height()) ? (l < r) : (l->height() < r->height());
              });
    // remove duplicates
    auto last = std::unique(result.begin(), result.end());
    // remove shrinked values
    result.erase(last, result.end());
    return result;
}

CltlTranslator::Node *CltlTranslator::_build_node(const FormulaList &terms) {
    FormulaList canonical = _unique_sort(terms);
    // search for a pre-existing instance of the node
    for (auto n : _nodes) {
        if (n->terms() == canonical) return n;
    }

    // build a new instance and stores its pointer
    Node *n = new Node(canonical);

    if (n->is_consistent())
        _transition_system.add_state(n);
    _nodes.push_back(n);
    return n;
}

CltlTranslator::NodeList CltlTranslator::_build_epsilon_successors(Node *node) {
    // take the formula with the highest height out of the subset to be reduced
    CltlFormulaPtr f = 0;
    FormulaList leftover(node->terms());

    for (long i = node->terms().size() - 1; i >= 0; --i) {
        // only binary operators must be reduced
        if (node->terms()[i]->formula_type() == CltlFormula::kBinaryOperator) {
            f = node->terms()[i];
            leftover.erase(leftover.begin() + i);
            break;
        }
    }

    if (!f) return {};

    BinaryOperator *bo = static_cast<BinaryOperator*>(f.get());
    NodeList successors;

    switch (bo->operator_type()) {
        // (f = f1 || f2) => [_,_,_]-> (f1)
        //                   [_,_,_]-> (f2)
        case BinaryOperator::kOr: {
            Node *s0 = _build_node(_insert(leftover, {bo->left()}));
            if (s0->is_consistent()) {
                _transition_system.add_transition(node, s0, new TransitionLabel());
                successors.push_back(s0);
            }

            Node *s1 = _build_node(_insert(leftover, {bo->right()}));
            if (s1->is_consistent()) {
                _transition_system.add_transition(node, s1, new TransitionLabel());
                successors.push_back(s1);
            }

            return successors;
        }

        // (f = f1 && f2) => [_,_,_]-> (f1, f2)
        case BinaryOperator::kAnd: {
            Node *s0 = _build_node(_insert(leftover, {bo->left(), bo->right()}));
            if (s0->is_consistent()) {
                _transition_system.add_transition(node, s0, new TransitionLabel());
                successors.push_back(s0);
            }

            return successors;
        }

        // (f = f1 U f2) => [_,_,_]-> (f2)
        //                  [_,_,f]-> (f1, X(f))
        case BinaryOperator::kUntil: {
            Node *s0 = _build_node(_insert(leftover, {bo->right()}));
            if (s0->is_consistent()) {
                _transition_system.add_transition(node, s0, new TransitionLabel());
                successors.push_back(s0);
            }

            Node *s1 = _build_node(_insert(leftover, {bo->left(), bo->creator()->make_next(f)}));
            if (s1->is_consistent()) {
                _transition_system.add_transition(node, s1, new TransitionLabel({},{},f));
                successors.push_back(s1);
            }

            return successors;
        }

        // (f = f1 R f2) => [_,_,_]-> (f1, f2)
        //                  [_,_,_]-> (f2, X(f))
        case BinaryOperator::kRelease: {
            Node *s0 = _build_node(_insert(leftover, {bo->left(), bo->right()}));
            if (s0->is_consistent()) {
                _transition_system.add_transition(node, s0, new TransitionLabel());
                successors.push_back(s0);
            }

            Node *s1 = _build_node(_insert(leftover, {bo->right(), bo->creator()->make_next(f)}));
            if (s1->is_consistent()) {
                _transition_system.add_transition(node, s1, new TransitionLabel());
                successors.push_back(s1);
            }

            return successors;
        }

        // (f = f1 UN f2) => [_,r,_]-> (f2)
        //                   [_,_,f]-> (f1, X(f))
        //                   [_,ic,f]-> (X(f))
        case BinaryOperator::kCostUntil: {
            std::size_t current_counter = _counters_maps[f];
            std::vector<std::string> counters(_nb_counters, "");

            Node *s0 = _build_node(_insert(leftover, {bo->right()}));
            if (s0->is_consistent()) {
                counters[current_counter] = "r";
                _transition_system.add_transition(node, s0, new TransitionLabel({},counters));
                successors.push_back(s0);
            }

            Node *s1 = _build_node(_insert(leftover, {bo->left(), bo->creator()->make_next(f)}));
            if (s1->is_consistent()) {
                counters[current_counter] = "";
                _transition_system.add_transition(node, s1, new TransitionLabel({},counters,f));
                successors.push_back(s1);
            }

            Node *s2 = _build_node(_insert(leftover,{bo->creator()->make_next(f)}));
            if (s2->is_consistent()) {
                counters[current_counter] = "ic";
                _transition_system.add_transition(node, s2, new TransitionLabel({},counters,f));
                successors.push_back(s2);
            }

            return successors;
        }

        // (f = f1 RN f2) => [_,r,_]-> (f1,f2)
        //                   [_,_,_]-> (f2, X(f))
        //                   [_,ic,_]-> (X(f))
        case BinaryOperator::kCostRelease: {
            std::size_t current_counter = _counters_maps[f];
            std::vector<std::string> counters(_nb_counters, "");

            Node *s0 = _build_node(_insert(leftover, {bo->left(), bo->right()}));
            if (s0->is_consistent()) {
                counters[current_counter] = "r";
                _transition_system.add_transition(node, s0, new TransitionLabel({},counters));
                successors.push_back(s0);
            }

            Node *s1 = _build_node(_insert(leftover, {bo->right(), bo->creator()->make_next(f)}));
            if (s1->is_consistent()) {
                counters[current_counter] = "";
                _transition_system.add_transition(node, s1, new TransitionLabel({},counters));
                successors.push_back(s1);
            }

            Node *s2 = _build_node({bo->creator()->make_next(f)});
            if (s2->is_consistent()) {
                counters[current_counter] = "ic";
                _transition_system.add_transition(node, s2, new TransitionLabel({},counters));
                successors.push_back(s2);
            }

            return successors;
        }
    }
}

CltlTranslator::Node *CltlTranslator::_build_actual_successor(Node *node) {
    FormulaList propositions;
    FormulaList successor_terms;

    for (auto f : node->terms()) {
        // formulae of type f = X(f1) will be transformed to f1 in the successor
        if (f->formula_type() == CltlFormula::kUnaryOperator) {
            UnaryOperator *uo = static_cast<UnaryOperator*>(f.get());
            if (uo->operator_type() == UnaryOperator::kNext) {
                successor_terms.push_back(uo->operand());
                continue;
            }
        }

        // reduced non-next formulae must be satisfied to move to the successor
        propositions.push_back(f);
    }

    Node *suc = _build_node(successor_terms);
    _transition_system.add_transition(node, suc, new TransitionLabel(propositions));
    return suc;
}

void CltlTranslator::_build_transition_system() {
    _to_be_reduced.push(_build_node({_formula}));
    _states.push_back(_build_node({_formula}));

    while (!(_to_be_reduced.empty() and _to_be_fired.empty())) {
        _process_reduce();
        _process_fire();
    }
}

void CltlTranslator::_process_reduce() {
    while (!_to_be_reduced.empty()) {
        // fetch the next pseudo-state to be reduced
        Node *s = _to_be_reduced.top();
        _to_be_reduced.pop();

        if (s->is_reduced())
            continue;
        s->set_reduced();

        // build the epsilon successors of `s` and put it to the reduce stack
        NodeList successors = _build_epsilon_successors(s);
        if (successors.empty())
            _to_be_fired.push(s);
        else for (auto t : successors) {
            _to_be_reduced.push(t);
        }
    }
}

void CltlTranslator::_process_fire() {
    while (!_to_be_fired.empty()) {
        // fetch the next pseudo-state to be fired
        Node *s = _to_be_fired.top();
        _to_be_fired.pop();

        // build the actual successor of `s`
        Node *t = _build_actual_successor(s);
        if (std::find(_states.begin(), _states.end(), t) == _states.end())
            _states.push_back(t);

        // `t` should never be empty (ie. end of the word), but just in case, we won't
        // add it to the reduce stack since it would cause a null pointer exception
        if (!t->terms().empty())
            _to_be_reduced.push(t);
    }
}

CltlTranslator::FormulaList CltlTranslator::_insert(const FormulaList &list,
                                                    const std::initializer_list<CltlFormulaPtr> &add_list) const {
    FormulaList result(list);
    result.insert(result.end(), add_list);
    return result;
}

bool CltlTranslator::Node::is_consistent() const {
    FormulaList truths;
    FormulaList negations;

    for (auto f : _terms) {
        if (f->formula_type() == CltlFormula::kUnaryOperator) {
            UnaryOperator *uo = static_cast<UnaryOperator*>(f.get());
            if (uo->operator_type() == UnaryOperator::kNot) {
                negations.push_back(uo->operand());
                continue;
            }
        } else if (f->formula_type() == CltlFormula::kConstantExpression) {
            ConstantExpression *ce = static_cast<ConstantExpression*>(f.get());
            if (!ce->value())
                return false;
        }
        truths.push_back(f);
    }

    for (auto f : negations) {
        if (std::find(truths.begin(), truths.end(), f) != truths.end())
            return false;
    }

    return true;
}

const std::string CltlTranslator::Node::dump(const std::string &sep) const {
    std::string node_name = "";
    for (const auto t : _terms) {
        node_name += "[" + t->dump() + "]" + sep + " ";
    }
    return node_name;
}

const std::string CltlTranslator::TransitionLabel::dump() const {
    std::string result = "";
    if (propositions.empty()) {
        result += "true";
    } else {
        auto it = propositions.begin();
        result += (*it)->dump();
        for (++it; it != propositions.end(); ++it) {
            result += " && " + (*it)->dump();
        }
    }

    result += "\\n";
    for (auto c : counter_actions) {
        result += c + "/";
    }
    if (postponed != nullptr) {
        result += "\\n";
        result += "PP { " + postponed->dump() + " }";
    }
    return result;
}

}  // namespace automata
}  // namespace spaction
