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

#ifndef SPACTION_INCLUDE_SPOTCHECK_H_
#define SPACTION_INCLUDE_SPOTCHECK_H_

#include <string>

#include <spot/tgba/tgba.hh>

#include "CltlFormula.h"
#include "automata/TGBA2CA.h"

namespace spaction {

// @todo expose it?
// uses spot to check a LTL formula against a DVE model
// @remarks
//          the LTL formula is tested as is. It is the responsibility of the user to negate the
//          formula if necessary.
// @param formula       a LTL formula
// @param modelfile     the path to the DVE model which \a formula is tested against
// @return              true iff \a formula holds on no execution of \a model (empty product)
bool spot_dve_check(const std::string &formula, const std::string &modelfile);

/// finds the min bound of the given formula over the given model
/// in practice, uses CLTL[<=] formulae
unsigned int find_bound_min(const CltlFormulaPtr &formula, const std::string &modelname);
/// finds the min bound of the given formula over the given model
/// in practice, uses CLTL[>] formulae
unsigned int find_bound_max(const CltlFormulaPtr &formula, const std::string &modelname);

/// use a tgba as a CounterAutomaton
automata::tgba_ca *load_formula(const std::string &formula);

}  // namespace spaction

#endif  // SPACTION_INCLUDE_SPOTCHECK_H_
