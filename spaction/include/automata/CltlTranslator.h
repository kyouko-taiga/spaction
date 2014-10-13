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

#ifndef SPACTION_INCLUDE_CLTLTRANSLATOR_H_
#define SPACTION_INCLUDE_CLTLTRANSLATOR_H_

#include <stack>
#include <string>
#include <vector>

#include "CltlFormula.h"
#include "automata/RegisterAutomaton.h"
#include "automata/TransitionSystem.h"

namespace spaction {
namespace automata {

struct Node;
struct TransitionLabel;

class CltlTranslator {
public:
    /// Type definition for a set of CLTL formulae.
    typedef std::vector<CltlFormulaPtr> FormulaList;

    explicit CltlTranslator(const CltlFormulaPtr &formula);

    void build_automaton();

private:
    /// Helper class representing the states of the temporary transition system.
    ///
    /// This struct is used to represent states and pseudo-states of the the temporary transition
    /// system, in the process of translating a CLTL formula into a counter automaton. So called
    /// pseudo-states are those obtained by building the epsilon-transitions from actual states.
    class Node {
    public:
        /// `terms` is assumed to be sorted according to `_unique_sort` (see below)
        /// this constructor is therefore not supposed to be called outside of `_build_node`
        explicit inline Node(const CltlTranslator::FormulaList &terms) :
            _terms(terms), _is_reduced(false) {
        }

        const inline CltlTranslator::FormulaList &terms() const { return _terms; }

        void inline set_reduced(bool reduced=true) { _is_reduced = reduced; }
        bool inline is_reduced() const { return _is_reduced; }

        bool is_consistent() const;

        const std::string dump(const std::string &sep=",") const;

    private:
        /// List of subformulae corresponding to this pseudo-state.
        /// @remarks
        ///     This list remains always ordered by the height of the formulae it contains, such that
        ///     the latest element of the list is the biggest formula.
        const CltlTranslator::FormulaList _terms;

        bool _is_reduced;
    };

    /// Helper struct representing the alphabet of the temporary transition system.
    ///
    /// This struct is used to represent the alphabet of the transitions of the temporary transition
    /// system, in the process of translating a CLTL formula into a counter automaton. Each letter
    /// of this alphabet is composed of the set of propositions to be satisfied, the set of actions
    /// on the counters, and the optional postponed condition marking.
    struct TransitionLabel {
        /// Set of propositions that needs to be satisfied to fire the transition.
        CltlTranslator::FormulaList propositions;
        /// Vector of actions on the counters
        std::vector<std::string> counter_actions;
        /// Optional until formula that would have been postponed.
        CltlFormulaPtr postponed;

        explicit inline TransitionLabel(const CltlTranslator::FormulaList &propositions={},
                                        const std::vector<std::string> &counter_actions={},
                                        const CltlFormulaPtr &postponed=nullptr) :
            propositions(propositions), counter_actions(counter_actions), postponed(postponed) {
        }
    };

    typedef std::vector<Node*> NodeList;

    /// Stores the formula being translated by this translator.
    const CltlFormulaPtr _formula;

    /// Stores the set of pseudo-states built during the construction of the transition system, to
    /// ensure their uniqueness.
    NodeList _nodes;

    /// Stores the temporar transition system that is used to build the automata.
    UndeterministicTransitionSystem<Node*, TransitionLabel*> _transition_system;

    std::size_t _nb_counters;

    std::stack<Node*> _to_be_reduced;
    std::stack<Node*> _to_be_fired;
    std::vector<Node*> _states;

    /// a helper function that returns from a list of terms a sorted set, to ease comparison.
    /// Sorting is done according to height, so that the last element of the list
    /// is also (one of) the highest.
    static FormulaList _unique_sort(const CltlTranslator::FormulaList &terms);
    /// Either builds or returns an existing node for the given set of `terms`.
    Node *_build_node(const FormulaList &terms);

    /// Builds the epsilon successors of the given `node` and updates the transition system
    /// accordingly.
    NodeList _build_epsilon_successors(Node *node);

    /// Builds the actual successor of the given `node` (ie. by consuming the propositions to
    /// be satisfied) and updates the transition system accordingly.
    Node *_build_actual_successor(Node *node);

    /// Builds the temporary transition system out of the formula.
    void _build_transition_system();
    void _process_reduce();
    void _process_fire();

    /// Helper method that inserts a formula into a FormulaList and keeps the result sorted.
    FormulaList _insert(const FormulaList &list, const std::initializer_list<CltlFormulaPtr> &add_list) const;
};

}  // namespace automata
}  // namespace spaction

#endif  // defined SPACTION_INCLUDE_CLTLTRANSLATOR_H_
