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

#include <set>

#include "cltl.h"
#include "cra/CostRegisterAutomaton.h"

namespace spaction {
namespace automata {

/// Helper class to build a Generalized Büchi Automata (GBA) from a CLTL formula.
///
/// This class represents a single node within the tree that denotes a set of terms and its
/// (epsilon-)transition. Each node is a tuple <T,S> where T is a set of terms and S a the set
/// of successor nodes. Furthermore, each successor is labeled by the sub-alphabet that may lead
/// to, together with the potential postponed constraints.
class Node {
public:
    typedef std::set<cltl_formula*> Terms;
    typedef FormulaeSet::iterator TermsIterator;
    typedef FormulaeSet::const_iterator TermsConstIterator;

    typedef std::pair<Node*, Terms> Successor;
    typedef std::set<Successor> Successors;

    explicit Node(Terms&& terms);

    Terms terms();
    TermsIterator terms_iterator();
    TermsConstIterator terms_iterator() const;

    Successors successors();

};

class CltlTranslator {
    
};

}  // namespact automata
}  // namespact spaction

#endif  // defined SPACTION_INCLUDE_CLTLTRANSLATOR_H_
