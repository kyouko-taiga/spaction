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
    typedef std::vector<Node*> NodeList;

    /// Stores the formula being translated by this translator.
    const CltlFormulaPtr _formula;

    /// Stores the set of pseudo-states built during the construction of the transition system, to
    /// ensure their uniqueness.
    NodeList _nodes;

    /// Stores the temporar transition system that is used to build the automata.
    UndeterministicTransitionSystem<TransitionLabel*> _transition_system;

    /// Stores the number of counters.
    std::size_t _nb_counters;

    std::stack<Node*> _to_be_reduced;
    std::stack<Node*> _to_be_fired;
    std::vector<Node*> _states;

    /// Either builds or returns an existing node for the given set of `terms`.
    Node *_build_node(const FormulaList &terms);

    /// Builds the epsilon successors of the given `node` and updates the transition system
    /// accordingly.
    NodeList _build_epsilon_successors(Node *node);

    /// Builds the actual successor of the given `node` (ie. by consuming the propositions to
    /// be satisfied) and updates the transition system accordingly.
    Node *_build_actual_successor(Node *node);

    void _build_transition_system();
    void _process_reduce();
    void _process_fire();

    /// Helper method that inserts a formula into a FormulaList and keeps the result sorted.
    FormulaList _insert(const FormulaList &list, const CltlFormulaPtr &formula) const;
};

/// Helper class to build a Transition Generalized Büchi Automata (TGBA) from a CLTL formula.
///
/// This class represents a single node within the tree that denotes a set of terms and its
/// (epsilon-)transition. Each node is a tuple <T,S> where T is a set of terms and S a the set
/// of successor nodes. Furthermore, each successor is labeled by the sub-alphabet that may lead
/// to, together with the potential postponed constraints.
struct Node {
    /// Name of the corresponding state in the transition system.
    const std::string state;

    /// List of subformulae corresponding to this pseudo-state.
    /// @remarks
    ///     This list remains always ordered by the height of the formulae it contains, such that
    ///     the latest element of the list is the biggest formula.
    const CltlTranslator::FormulaList terms;

    /// Stores wether this node has already been reduced.
    bool is_reduced;

    explicit inline Node(const std::string &state, const CltlTranslator::FormulaList &terms) :
        state(state), terms(terms), is_reduced(false) {
    }
};

struct TransitionLabel {
    /// Set of propositions that needs to be satisfied to fire the transition.
    CltlTranslator::FormulaList propositions;
    /// Vector of actions on the counters
    std::vector<std::string> counter_actions;
    /// Optional until formula that would have been postponed.
    CltlFormulaPtr postponed;

    explicit inline TransitionLabel(const CltlTranslator::FormulaList &propositions={},
                                    const std::vector<std::string> &counter_actions={},
                                    const CltlFormulaPtr &postoned=0) :
        propositions(propositions), counter_actions(counter_actions), postponed(postoned) {
    }
};

}  // namespact automata
}  // namespact spaction

#endif  // defined SPACTION_INCLUDE_CLTLTRANSLATOR_H_
