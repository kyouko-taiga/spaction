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
    _formula(formula->to_nnf()), _nb_acceptances(0), _nb_counters(0) {
        this->map_costop_to_counters(_formula);
        _automaton = CounterAutomaton<Node*, FormulaList, UndeterministicTransitionSystem>(_nb_counters, _nb_acceptances);
}

void CltlTranslator::build_automaton() {
    _build_transition_system();
    _build_automaton();
}

void CltlTranslator::map_costop_to_counters(const CltlFormulaPtr &f) {
    switch (f->formula_type()) {
        case CltlFormula::kUnaryOperator:
            this->map_costop_to_counters(static_cast<UnaryOperator*>(f.get())->operand());
            break;
        case CltlFormula::kBinaryOperator: {
            BinaryOperator *fbin = static_cast<BinaryOperator*>(f.get());
            switch (fbin->operator_type()) {
                case BinaryOperator::kCostUntil:
                case BinaryOperator::kCostRelease:
                    if (_counters_maps.count(f) != 0)
                        throw f->dump() + " already has a counter associated";
                    _counters_maps[f] = _nb_counters++;
                    // no break here, as the following also applies to Cost operators
                case BinaryOperator::kUntil:
                    if (_acceptances_maps.count(f) != 0)
                        throw f->dump() + " already has an acceptance condition associated";
                    _acceptances_maps[f] = _nb_acceptances++;
                    break;
                default:
                    break;
            }
            // recursive calls
            this->map_costop_to_counters(fbin->left());
            this->map_costop_to_counters(fbin->right());
            break;
        }
        default:
            // nothing to do
            break;
    }
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
                _transition_system.add_transition(node, s0, new TransitionLabel({}, CounterOperationList(_nb_counters)));
                successors.push_back(s0);
            }

            Node *s1 = _build_node(_insert(leftover, {bo->right()}));
            if (s1->is_consistent()) {
                _transition_system.add_transition(node, s1, new TransitionLabel({}, CounterOperationList(_nb_counters)));
                successors.push_back(s1);
            }

            return successors;
        }

        // (f = f1 && f2) => [_,_,_]-> (f1, f2)
        case BinaryOperator::kAnd: {
            Node *s0 = _build_node(_insert(leftover, {bo->left(), bo->right()}));
            if (s0->is_consistent()) {
                _transition_system.add_transition(node, s0, new TransitionLabel({}, CounterOperationList(_nb_counters)));
                successors.push_back(s0);
            }

            return successors;
        }

        // (f = f1 U f2) => [_,_,_]-> (f2)
        //                  [_,_,f]-> (f1, X(f))
        case BinaryOperator::kUntil: {
            Node *s0 = _build_node(_insert(leftover, {bo->right()}));
            if (s0->is_consistent()) {
                _transition_system.add_transition(node, s0, new TransitionLabel({}, CounterOperationList(_nb_counters)));
                successors.push_back(s0);
            }

            Node *s1 = _build_node(_insert(leftover, {bo->left(), bo->creator()->make_next(f)}));
            if (s1->is_consistent()) {
                _transition_system.add_transition(node, s1, new TransitionLabel({}, CounterOperationList(_nb_counters), f));
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
                _transition_system.add_transition(node, s1, new TransitionLabel({}, CounterOperationList(_nb_counters)));
                successors.push_back(s1);
            }

            return successors;
        }

        // (f = f1 UN f2) => [_,r,_]-> (f2)
        //                   [_,_,f]-> (f1, X(f))
        //                   [_,ic,f]-> (X(f))
        case BinaryOperator::kCostUntil: {
            std::size_t current_counter = _counters_maps[f];
            CounterOperationList counters(_nb_counters, _e());

            Node *s0 = _build_node(_insert(leftover, {bo->right()}));
            if (s0->is_consistent()) {
                counters[current_counter] = _r();
                _transition_system.add_transition(node, s0, new TransitionLabel({}, counters));
                successors.push_back(s0);
            }

            Node *s1 = _build_node(_insert(leftover, {bo->left(), bo->creator()->make_next(f)}));
            if (s1->is_consistent()) {
                counters[current_counter] = _e();
                _transition_system.add_transition(node, s1, new TransitionLabel({}, counters, f));
                successors.push_back(s1);
            }

            Node *s2 = _build_node(_insert(leftover,{bo->creator()->make_next(f)}));
            if (s2->is_consistent()) {
                counters[current_counter] = _ic();
                _transition_system.add_transition(node, s2, new TransitionLabel({}, counters, f));
                successors.push_back(s2);
            }

            return successors;
        }

        // (f = f1 RN f2) => [_,r,_]-> (f1,f2)
        //                   [_,_,_]-> (f2, X(f))
        //                   [_,ic,_]-> (X(f))
        case BinaryOperator::kCostRelease: {
            std::size_t current_counter = _counters_maps[f];
            CounterOperationList counters(_nb_counters, _e());

            Node *s0 = _build_node(_insert(leftover, {bo->left(), bo->right()}));
            if (s0->is_consistent()) {
                counters[current_counter] = _r();
                _transition_system.add_transition(node, s0, new TransitionLabel({}, counters));
                successors.push_back(s0);
            }

            Node *s1 = _build_node(_insert(leftover, {bo->right(), bo->creator()->make_next(f)}));
            if (s1->is_consistent()) {
                counters[current_counter] = _e();
                _transition_system.add_transition(node, s1, new TransitionLabel({}, counters));
                successors.push_back(s1);
            }

            Node *s2 = _build_node({bo->creator()->make_next(f)});
            if (s2->is_consistent()) {
                counters[current_counter] = _ic();
                _transition_system.add_transition(node, s2, new TransitionLabel({}, counters));
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
    _transition_system.add_transition(node, suc, new TransitionLabel(propositions, CounterOperationList(_nb_counters)));
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

void CltlTranslator::_build_automaton() {
    // the TS underlying the real automaton
    auto automaton_ts = _automaton.transition_system();

    // the initial state
    Node *initial_node = _build_node({_formula});
    automaton_ts->add_state(initial_node);
    _automaton.set_initial_state(initial_node);

    _to_remove_epsilon.push(initial_node);
    while (!_to_remove_epsilon.empty()) {
        _process_remove_epsilon();
    }
}

void CltlTranslator::_process_remove_epsilon() {
    // fetch the next state to be processed
    Node *s = _to_remove_epsilon.top();
    _to_remove_epsilon.pop();

    // on fait un parcours en profondeur depuis 's'
    // on coupe une branche dès qu'on atteint un état réduit
    _process_remove_epsilon(s, s, {});
    _done_remove_epsilon.insert(s);
}

void CltlTranslator::_process_remove_epsilon(Node *source, Node *s,
                                             const std::vector<TransitionLabel*> &trace) {
    // base case
    if (s->is_reduced()) {
        for (auto succ: _transition_system(s).successors()) {
            std::vector<TransitionLabel*> new_trace;
            new_trace.push_back(succ->label());
            _add_nonepsilon_transition(source, succ->sink(), new_trace);
            if (_done_remove_epsilon.count(succ->sink()) == 0) {
                _to_remove_epsilon.push(succ->sink());
            }
        }
        return;
    }

    // recursive case
    for (auto succ: _transition_system(s).successors()) {
        std::vector<TransitionLabel*> new_trace = trace;
        new_trace.push_back(succ->label());
        _process_remove_epsilon(source, succ->sink(), new_trace);
    }
}

void CltlTranslator::_add_nonepsilon_transition(Node *source, Node *sink,
                                                const std::vector<TransitionLabel*> &trace) {
    // add source and sink to the transition system
    _automaton.transition_system()->add_state(source);
    _automaton.transition_system()->add_state(sink);

    // build counter actions
    CounterOperationList counter_actions(_nb_counters);
    for (auto t: trace) {
        assert(t->counter_actions.size() == _nb_counters);
        for (std::size_t i = 0 ; i != _nb_counters ; ++i) {
            // there should not be several actions on the same counter along a single trace
            assert(!(counter_actions[i] and t->counter_actions[i]));

            counter_actions[i] = counter_actions[i] | t->counter_actions[i];
        }
    }

    // build label
    // only the last element should have a proposition (thank you, lambda functions)
    assert([&trace](void){
                auto it = std::find_if(trace.begin(), trace.end(), [](TransitionLabel *t) { return !(t->propositions.empty()); });
                return (it == trace.end()) or (++it == trace.end()); } ());

    auto it = std::find_if(trace.begin(), trace.end(),
                           [](TransitionLabel *t) { return !(t->propositions.empty()); } );
    FormulaList props;
    if (it != trace.end())
        props = (*it)->propositions;

    // build acceptance conditions
    std::set<std::size_t> accs;
    // @TODO using std::generate would be better
    for (std::size_t i = 0 ; i != _nb_acceptances ; ++i) {
        accs.insert(i);
    }
    for (auto t: trace) {
        auto it = _acceptances_maps.find(t->postponed);
        if (it != _acceptances_maps.end())
            accs.erase(it->second);
    }

    // add into the automaton
    _automaton.transition_system()->add_transition(source, sink,
                                                   _automaton.make_label(props, {counter_actions}, accs));
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
        result += print_counter_operation(c);
        result += "/";
    }
    if (postponed != nullptr) {
        result += "\\n";
        result += "PP { " + postponed->dump() + " }";
    }
    return result;
}

}  // namespace automata
}  // namespace spaction

namespace std {

ostream &operator<<(ostream &os, const spaction::automata::CltlTranslator::FormulaList &fl) {
    for (auto f : fl) {
        os << f;
        os << ",";
    }
    return os;
}

ostream &operator<<(ostream &os, const spaction::automata::CltlTranslator::Node &n) {
    os << n.dump();
    return os;
}

ostream &operator<<(ostream &os, const spaction::automata::CltlTranslator::TransitionLabel &l) {
    os << l.dump();
    return os;
}

}  // namespace std
