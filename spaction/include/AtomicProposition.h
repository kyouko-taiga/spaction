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

#ifndef SPACTION_INCLUDE_ATOMICPROPOSITION_H_
#define SPACTION_INCLUDE_ATOMICPROPOSITION_H_

#include "CltlFormula.h"

namespace spaction {

class CltlFormulaFactory;
class CltlFormulaVisitor;

class AtomicProposition : public CltlFormula {
 public:
    /// Copy construction is forbidden.
    AtomicProposition(const AtomicProposition &) = delete;
    /// Copy assignement is forbidden.
    AtomicProposition &operator=(const AtomicProposition &) = delete;

    inline const FormulaType formula_type() const override { return kAtomicProposition; }

    /// Returns whether or not `rhs` is syntactically equivalent to this formula.
    virtual bool syntactic_eq(const CltlFormula &rhs) const;

    /// Returns the string identifying this atomic proposition.
    inline const std::string &value() const { return _value; }

    void accept(CltlFormulaVisitor &visitor) override;

    inline std::size_t height() const { return 1; }

    inline bool is_infltl() const { return true; }
    inline bool is_supltl() const { return true; }
    inline bool is_propositional() const { return true; }
    inline bool is_nnf() const { return true; }

    std::string dump() const override;

 protected:
    explicit AtomicProposition(const std::string &value, CltlFormulaFactory *creator);
    ~AtomicProposition() { }

 private:
    friend class CltlFormulaFactory;
    std::string _value;
};

}  // namespace spaction

#endif  // SPACTION_INCLUDE_ATOMICPROPOSITION_H_
