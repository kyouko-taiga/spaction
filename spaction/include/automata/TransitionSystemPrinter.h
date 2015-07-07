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

#ifndef SPACTION_INCLUDE_AUTOMATA_TRANSITIONSYSTEMPRINTER_H_
#define SPACTION_INCLUDE_AUTOMATA_TRANSITIONSYSTEMPRINTER_H_

#include <fstream>
#include <map>

#include "TransitionSystem.h"

namespace spaction {
namespace automata {

template<typename Q, typename S, typename Derived, typename Iterator> class TSPrinter {
 public:
    explicit TSPrinter(TransitionSystem<Q, S, Derived, Iterator> &s): _system(s) { }
    ~TSPrinter() { }

    void dump(const std::string &filename) {
        std::ofstream ofs(filename);
        this->dump(ofs);
        ofs.close();
    }

    void dump(std::ostream &os) {
        // initiate dot file
        os << "digraph G {" << std::endl;
        //        o << "0 [label=\"\", style=invis, height=0];" << std::endl;
        //        o << "0 -> 1;" << std::endl;

        // export system's nodes
        unsigned int i = 1;
        std::map<Q, unsigned int> node_map;
        for (auto current_node : _system.states()) {
            // it is of type pair<iterator, bool>
            auto it = node_map.insert(std::make_pair(current_node, i));
            // increment i if current_node was not already in node_map
            if (it.second) ++i;

            os << it.first->second << " [label=\"";
            _system.print_state(os, current_node);
            os << "\" ];" << std::endl;
            for (auto current_transition : _system(current_node).successors()) {
                auto jt = node_map.insert(std::make_pair(current_transition->sink(), i));
                // increment i if current_transition->sink was not already in node_map
                if (jt.second) ++i;

                os << it.first->second << "->" << jt.first->second << " [label=\"";
                _system.print_label(os, current_transition->label());
                os << "\" ];" << std::endl;
            }
        }

        // end of graph definition
        os << "}" << std::endl;
    }

 private:
    TransitionSystem<Q, S, Derived, Iterator> &_system;
};

/// a helper class to handle pointers
template <typename A>
struct PrinterHelper {
    static void print(std::ostream &os, const A &a) { os << a; }
};

/// partial specialization that dereferences given pointer before printing
template <typename A>
struct PrinterHelper<A*> {
    static void print(std::ostream &os, const A * const a) { PrinterHelper<A>::print(os, *a); }
};

/// a specialization to handle pairs
template<typename A, typename B>
struct PrinterHelper<std::pair<A, B>> {
    static void print(std::ostream &os, const std::pair<A, B> &p) {
        os << "(";
        PrinterHelper<A>::print(os, p.first);
        os << ",";
        PrinterHelper<B>::print(os, p.second);
        os << ")";
    }
};

template<typename Q, typename S, typename Derived, typename Iterator>
TSPrinter<Q, S, Derived, Iterator>
make_ts_printer(TransitionSystem<Q, S, Derived, Iterator> &s) {
    return TSPrinter<Q, S, Derived, Iterator>(s);
}

}  // namespace automata
}  // namespace spaction

#endif  // SPACTION_INCLUDE_AUTOMATA_TRANSITIONSYSTEMPRINTER_H_
