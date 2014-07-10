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

#ifndef __spaction__binop_h__
#define __spaction__binop_h__

#include "cltl.h"

typedef enum {
  AND,
  OR,
  UNTIL,
  RELEASE,
  COST_UNTIL,
  COST_RELEASE
}
binop_type;

class binop : public cltl_formula {
public:
  binop (binop_type, const cltl_formula *, const cltl_formula *);
  ~binop ();
  
  binop (const binop &) = delete;
  binop & operator= (const binop &) = delete;
  
  cltl_formula * clone () const;
  
  void accept (cltl_visitor & v) const override
  { v.visit (this); }
  
  std::string dump () const;
  
  binop_type get_type () const { return type_; }
  
  const cltl_formula * left () const
  {
    return l_;
  }
  
  const cltl_formula * right () const
  {
    return r_;
  }

private:
  binop_type type_;
  const cltl_formula * l_;
  const cltl_formula * r_;
};

#endif // define __spaction__binop_h__
