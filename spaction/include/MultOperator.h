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

#ifndef SPACTION_INCLUDE_MULTOPERATOR_H_
#define SPACTION_INCLUDE_MULTOPERATOR_H_

#include <vector>

#include "CltlFormula.h"

namespace spaction {

class CltlFormulaFactory;
class CltlFormulaVisitor;

class MultOperator : public CltlFormula {
 public:
    enum MultOperatorType : char {
        kOr,
        kAnd
    };

    /// Copy construction is forbidden.
    MultOperator(const MultOperator &) = delete;
    /// Copy assignement is forbidden.
    MultOperator &operator=(const MultOperator &) = delete;

    inline const FormulaType formula_type() const override { return kMultOperator; }
    MultOperatorType operator_type() const { return _type; }

    inline const std::vector<CltlFormulaPtr> &childs() const { return _childs; }

    std::size_t hash() const override;
    /// Returns whether or not `rhs` is syntactically equivalent to this formula.
    virtual bool syntactic_eq(const CltlFormula &rhs) const;

    void accept(CltlFormulaVisitor &visitor) override;

    std::size_t height() const;

    bool is_infltl() const;
    bool is_supltl() const;
    bool is_propositional() const;
    bool is_nnf() const;

    virtual CltlFormulaPtr to_nnf() override;

    std::string dump() const override;

 protected:
    explicit MultOperator(MultOperatorType type, const std::vector<CltlFormulaPtr> &childs,
                          CltlFormulaFactory *creator);
    ~MultOperator() { }

 private:
    friend class CltlFormulaFactory;

    MultOperatorType _type;
    const std::vector<CltlFormulaPtr> _childs;
};

}  // namespace spaction

#endif  // SPACTION_INCLUDE_MULTOPERATOR_H_
