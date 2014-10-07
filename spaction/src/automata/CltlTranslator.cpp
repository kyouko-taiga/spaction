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
#include "UnaryOperator.h"

namespace spaction {
namespace automata {

CltlTranslator::CltlTranslator(const CltlFormulaPtr &formula) :
    _formula(formula->to_nnf()), _nb_counters(0) {
}

void CltlTranslator::build_automaton() {
    _build_transition_system();
}

CltlTranslator::Node *CltlTranslator::_build_node(const FormulaList &terms) {
    // search for a pre-existing instance of the node
    for (auto n : _nodes) {
        if (n->terms() == terms) return n;
    }

    // build a new instance and stores its pointer
    Node *n = new Node(terms);
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

    if (f) {
        BinaryOperator *bo = static_cast<BinaryOperator*>(f.get());
        switch (bo->operator_type()) {
            // (f = f1 || f2) => [_,_,_]-> (f1)
            //                   [_,_,_]-> (f2)
            case BinaryOperator::kOr: {
                Node *phi_left = _build_node(_insert(leftover, bo->left()));
                _transition_system.add_transition(node, phi_left, new TransitionLabel());

                Node *phi_right = _build_node(_insert(leftover, bo->right()));
                _transition_system.add_transition(node, phi_right, new TransitionLabel());

                return {phi_left, phi_right};
            }

            // (f = f1 && f2) => [_,_,_]-> (f1, f2)
            case BinaryOperator::kAnd: {
                Node *phi = _build_node(_insert(_insert(leftover, bo->left()), bo->right()));
                _transition_system.add_transition(node, phi, new TransitionLabel());
                return {phi};
            }

            // (f = f1 U f2) => [_,_,_]-> (f2)
            //                  [_,_,f]-> (f1, X(f))
            case BinaryOperator::kUntil: {
                Node *phi_0 = _build_node(_insert(leftover, bo->right()));
                _transition_system.add_transition(node, phi_0, new TransitionLabel());

                Node *phi_1 = _build_node(_insert(_insert(leftover, bo->left()),
                                                  bo->creator()->make_next(f)));
                _transition_system.add_transition(node, phi_1, new TransitionLabel({},{},f));

                return {phi_0, phi_1};
            }

            // (f = f1 R f2) => [_,_,_]-> (f1, f2)
            //                  [_,_,_]-> (f2, X(f))
            case BinaryOperator::kRelease: {
                Node *phi_0 = _build_node(_insert(_insert(leftover, bo->left()), bo->right()));
                _transition_system.add_transition(node, phi_0, new TransitionLabel());

                Node *phi_1 = _build_node(_insert(_insert(leftover, bo->right()),
                                                  bo->creator()->make_next(f)));
                _transition_system.add_transition(node, phi_1, new TransitionLabel());

                return {phi_0, phi_1};
            }

            // (f = f1 UN f2) => [_,r,_]-> (f2)
            //                   [_,_,f]-> (f1, X(f))
            //                   [_,ic,f]-> (X(f))
            case BinaryOperator::kCostUntil: {
                std::vector<std::string> counters(++_nb_counters, "");

                Node *phi_0 = _build_node(_insert(_insert(leftover, bo->left()), bo->right()));
                counters[_nb_counters-1] = "r";
                _transition_system.add_transition(node, phi_0, new TransitionLabel({},counters));

                Node *phi_1 = _build_node(_insert(_insert(leftover, bo->right()),
                                                  bo->creator()->make_next(f)));
                _transition_system.add_transition(node, phi_1, new TransitionLabel({},{},f));

                Node *phi_2 = _build_node({bo->creator()->make_next(f)});
                counters[_nb_counters-1] = "ic";
                _transition_system.add_transition(node, phi_2, new TransitionLabel({},counters,f));

                return {phi_0, phi_1, phi_2};
            }

            // (f = f1 RN f2) => [_,r,_]-> (f1,f2)
            //                   [_,_,_]-> (f2, X(f))
            //                   [_,ic,_]-> (X(f))
            case BinaryOperator::kCostRelease: {
                std::vector<std::string> counters(++_nb_counters, "");

                Node *phi_0 = _build_node(_insert(_insert(leftover, bo->left()), bo->right()));
                counters[_nb_counters-1] = "r";
                _transition_system.add_transition(node, phi_0, new TransitionLabel({},counters));
                
                Node *phi_1 = _build_node(_insert(_insert(leftover, bo->right()),
                                                  bo->creator()->make_next(f)));
                _transition_system.add_transition(node, phi_1, new TransitionLabel());

                Node *phi_2 = _build_node({bo->creator()->make_next(f)});
                counters[_nb_counters-1] = "ic";
                _transition_system.add_transition(node, phi_2, new TransitionLabel({},counters));
                
                return {phi_0, phi_1, phi_2};
            }
        }
    }

    // if no epsilon successors were built, simply return the empty set
    return {};
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

    Node *phi = _build_node(successor);
    _transition_system.add_transition(node, phi, new TransitionLabel(propositions));
    return phi;
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
        _states.push_back(t);

        // `t` should never be empty (ie. end of the word), but just in case, we won't
        // add it to the reduce stack since it would cause a null pointer exception
        if (!t->terms().empty())
            _to_be_reduced.push(t);
    }
}

CltlTranslator::FormulaList CltlTranslator::_insert(const FormulaList &list,
                                                    const CltlFormulaPtr &formula) const {
    FormulaList result(list);
    FormulaList::iterator it = result.begin();

    // seek for the position at which insert the new element
    while((it != result.end()) and (*it)->height() < formula->height())
        it++;

    result.insert(it, formula);
    return result;
}

}  // namespace automata
}  // namespace spaction
