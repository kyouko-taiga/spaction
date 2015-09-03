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

#ifndef SPACTION_INCLUDE_CLTL2SPOT_H_
#define SPACTION_INCLUDE_CLTL2SPOT_H_

#include <spot/ltlast/formula.hh>

#include "CltlFormula.h"

namespace spaction {

/// a function that converts a LTL formula in spaction representation to spot representation
/// @param      a LTL formula
/// @return     the same \a formula in spot format
const spot::ltl::formula *cltl2spot(const CltlFormulaPtr &formula);

/// a function that converts a LTL formula in spot representation to spaction representation
/// @param      a LTL formula
/// @param      a factory for the spaction formula
/// @return     the same \a formula in spaction format
CltlFormulaPtr spot2cltl(const spot::ltl::formula *formula, CltlFormulaFactory *factory);

}  // namespace spaction

#endif  // SPACTION_INCLUDE_CLTL2SPOT_H_
