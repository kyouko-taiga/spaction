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

#ifndef SPACTION_INCLUDE_TSPRINTER_H_
#define SPACTION_INCLUDE_TSPRINTER_H_

#include <fstream>

#include "TransitionSystem.h"

namespace spaction {
namespace automata {

template<typename Q, typename S> class TSPrinter {
 public:
    explicit TSPrinter(TransitionSystem<Q,S> &s): _system(s) { }
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

            os << it.first->second << " [label=\"" << current_node << "\" ];" << std::endl;
            for (auto current_transition : _system(current_node).successors()) {
                auto jt = node_map.insert(std::make_pair(current_transition->sink(), i));
                // increment i if current_transition->sink was not already in node_map
                if (jt.second) ++i;

                os << it.first->second << "->" << jt.first->second
                   << " [label=\"" << current_transition->label() << "\" ];"
                   << std::endl;
            }
        }

        // end of graph definition
        os << "}" << std::endl;
    }

 private:
    TransitionSystem<Q,S> &_system;
};

}  // namespace automata
}  // namespace spaction

#endif  // defined SPACTION_INCLUDE_TSPRINTER_H_
