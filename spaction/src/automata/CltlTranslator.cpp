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

#include "BinaryOperator.h"
#include "CltlFormulaFactory.h"
#include "UnaryOperator.h"

namespace spaction {
namespace automata {

CltlTranslator::CltlTranslator(const CltlFormulaPtr &formula) : _formula(formula), _nb_counters(0) {
}

void CltlTranslator::build_automaton() {
    _build_transition_system();
}

Node *CltlTranslator::_build_node(const FormulaList &terms) {
    // search for a pre-existing instance of the node
    for (auto n : _nodes) {
        if (n->terms == terms) return n;
    }

    std::string node_name = "";
    for (const auto t : terms) {
        node_name += "(" + t->dump() + "), ";
    }

    // build a new instance and stores its pointer
    Node *n = new Node(node_name, terms);
    _nodes.push_back(n);
    return n;
}

CltlTranslator::NodeList CltlTranslator::_build_epsilon_successors(Node *node) {
    // take the formula with the highest height out of the set to be reduced
    CltlFormulaPtr f = node->terms.back();
    FormulaList leftover(node->terms.begin(), node->terms.end() - 1);

    if (f->formula_type() == CltlFormula::kBinaryOperator) {
        BinaryOperator *bo = static_cast<BinaryOperator*>(f.get());
        switch (bo->operator_type()) {
            // (f = f1 || f2) => [_,_,_]-> (f1)
            //                   [_,_,_]-> (f2)
            case BinaryOperator::kOr: {
                Node *phi_left = _build_node(_insert(leftover, bo->left()));
                _transition_system.add_transition(node->state, phi_left->state,
                                                  new TransitionLabel());

                Node *phi_right = _build_node(_insert(leftover, bo->right()));
                _transition_system.add_transition(node->state, phi_right->state,
                                                  new TransitionLabel());

                return {phi_left, phi_right};
            }

            // (f = f1 && f2) => [_,_,_]-> (f1, f2)
            case BinaryOperator::kAnd: {
                Node *phi = _build_node(_insert(_insert(leftover, bo->left()), bo->right()));
                _transition_system.add_transition(node->state, phi->state, new TransitionLabel());
                return {phi};
            }

            // (f = f1 U f2) => [_,_,_]-> (f2)
            //                  [_,_,f]-> (f1, X(f))
            case BinaryOperator::kUntil: {
                Node *phi_0 = _build_node(_insert(leftover, bo->right()));
                _transition_system.add_transition(node->state, phi_0->state, new TransitionLabel());

                Node *phi_1 = _build_node(_insert(_insert(leftover, bo->left()),
                                                  bo->creator()->make_next(f)));
                _transition_system.add_transition(node->state, phi_1->state,
                                                  new TransitionLabel({},{},f));

                return {phi_0, phi_1};
            }

            // (f = f1 R f2) => [_,_,_]-> (f1, f2)
            //                  [_,_,_]-> (f2, X(f))
            case BinaryOperator::kRelease: {
                Node *phi_0 = _build_node(_insert(_insert(leftover, bo->left()), bo->right()));
                _transition_system.add_transition(node->state, phi_0->state, new TransitionLabel());

                Node *phi_1 = _build_node(_insert(_insert(leftover, bo->right()),
                                                  bo->creator()->make_next(f)));
                _transition_system.add_transition(node->state, phi_1->state, new TransitionLabel());

                return {phi_0, phi_1};
            }

            // (f = f1 UN f2) => [_,r,_]-> (f2)
            //                   [_,_,f]-> (f1, X(f))
            //                   [_,ic,f]-> (X(f))
            case BinaryOperator::kCostUntil: {
                std::vector<std::string> counters(++_nb_counters, "");

                Node *phi_0 = _build_node(_insert(_insert(leftover, bo->left()), bo->right()));
                counters[_nb_counters-1] = "r";
                _transition_system.add_transition(node->state, phi_0->state,
                                                  new TransitionLabel({},counters));

                Node *phi_1 = _build_node(_insert(_insert(leftover, bo->right()),
                                                  bo->creator()->make_next(f)));
                _transition_system.add_transition(node->state, phi_1->state,
                                                  new TransitionLabel({},{},f));

                Node *phi_2 = _build_node({bo->creator()->make_next(f)});
                counters[_nb_counters-1] = "ic";
                _transition_system.add_transition(node->state, phi_2->state,
                                                  new TransitionLabel({},counters,f));

                return {phi_0, phi_1, phi_2};
            }

            // (f = f1 RN f2) => [_,r,_]-> (f1,f2)
            //                   [_,_,_]-> (f2, X(f))
            //                   [_,ic,_]-> (X(f))
            case BinaryOperator::kCostRelease: {
                std::vector<std::string> counters(++_nb_counters, "");

                Node *phi_0 = _build_node(_insert(_insert(leftover, bo->left()), bo->right()));
                counters[_nb_counters-1] = "r";
                _transition_system.add_transition(node->state, phi_0->state,
                                                  new TransitionLabel({},counters));
                
                Node *phi_1 = _build_node(_insert(_insert(leftover, bo->right()),
                                                  bo->creator()->make_next(f)));
                _transition_system.add_transition(node->state, phi_1->state, new TransitionLabel());

                Node *phi_2 = _build_node({bo->creator()->make_next(f)});
                counters[_nb_counters-1] = "ic";
                _transition_system.add_transition(node->state, phi_2->state,
                                                  new TransitionLabel({},counters));
                
                return {phi_0, phi_1, phi_2};
            }
        }
    }

    // if no epsilon successors were built, simply return the empty set
    return {};
}

Node *CltlTranslator::_build_actual_successor(Node *node) {
    FormulaList propositions;
    FormulaList successor;

    for (auto f : node->terms) {
        // formulae of type f = X(f1) will be transformed to f1 in the successor
        if (f->formula_type() == CltlFormula::kUnaryOperator) {
            UnaryOperator *uo = static_cast<UnaryOperator*>(f.get());
            if (uo->operator_type() == UnaryOperator::kNext) {
                successor.push_back(uo->operand());
                continue;
            }
        }

        // reduced non-next formulae must be satisfied to move to the successor
        propositions.push_back(f);
    }

    Node *phi = _build_node(successor);
    _transition_system.add_transition(node->state, phi->state, new TransitionLabel(propositions));
    return phi;
}

void CltlTranslator::_build_transition_system() {
    // start by reducing the formula to translate
    _to_be_reduced.push(_build_node({_formula}));
    _states.push_back(_build_node({_formula}));
    enum {kReduce, kFire} loop_state = kReduce;

    while (!(_to_be_reduced.empty() and _to_be_fired.empty())) {
        switch (loop_state) {
            case kReduce: {
                // fetch the next pseudo-state to be reduced
                Node *s = _to_be_reduced.top();
                _to_be_reduced.pop();

                // continue to the next pseudo-state if we already reduced this one
                if (s->is_reduced) continue;
                s->is_reduced = true;

                // todo: check that `s` is consistant and skip it unless it is

                // build the epsilon successors of `s` and put it to the reduce stack
                NodeList successors = _build_epsilon_successors(s);
                if (successors.empty())
                    _to_be_fired.push(s);
                else for (auto t : successors) {
                    _to_be_reduced.push(t);
                }

                // change the loop state if we don't have any pseudo-state to reduce
                if (_to_be_reduced.empty())
                    loop_state = kFire;
                break;
            }

            case kFire:
                // fetch the next pseudo-state to be fired
                Node *s = _to_be_fired.top();
                _to_be_fired.pop();

                // build the actual successor of `s` and put it to the reduce stack
                Node *t = _build_actual_successor(s);
                _states.push_back(t);
                if (!t->terms.empty())
                    _to_be_reduced.push(t);

                // change the loop state if we don't have any pseudo-state to fire
                if (_to_be_fired.empty())
                    loop_state = kReduce;
                break;
        }
    }
}

CltlTranslator::FormulaList CltlTranslator::_insert(const FormulaList &list,
                                                    const CltlFormulaPtr &formula) const {
    FormulaList result(list);
    FormulaList::iterator it = result.begin();

    // seek for the position at which insert the new element
    if (it != result.end()) {
        while ((*it)->height() < formula->height())
            it++;
    }

    result.insert(it, formula);
    return result;
}

}  // namespace automata
}  // namespace spaction
