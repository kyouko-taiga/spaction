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
// limitations under the License.language governing permissions and
// limitations under the License.

#ifndef SPACTION_INCLUDE_VISITOR_H_
#define SPACTION_INCLUDE_VISITOR_H_

#include "cltl.h"

namespace spaction {

const cltl_formula *instantiate_inf(const cltl_formula *formula, int n);
const cltl_formula *instantiate_sup(const cltl_formula *formula, int n);

}  // namespace spaction

#endif  // SPACTION_INCLUDE_VISITOR_H_
