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

#ifndef SPACTION_INCLUDE_BDD_UTIL_H_
#define SPACTION_INCLUDE_BDD_UTIL_H_

#include <twa/bdddict.hh>

#include "automata/TransitionSystem.h"

namespace spaction {
namespace automata {

class DataBddDict final: public Data {
 public:
    explicit DataBddDict(spot::bdd_dict_ptr d): _dict(d) { }

    void destroy(void *for_me) {
        _dict->unregister_all_my_variables(for_me);
    }

 private:
    spot::bdd_dict_ptr _dict;
};

}  // namespace automata
}  // namespace spaction

#endif  // SPACTION_INCLUDE_BDD_UTIL_H_
