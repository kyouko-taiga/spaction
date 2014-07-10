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

#include "binop.h"

#include <iostream>

binop::binop (binop_type t,
              const cltl_formula * l,
              const cltl_formula * r)
: type_ (t)
, l_ (l->clone ())
, r_ (r->clone ())
{}

binop::~binop ()
{
  delete l_;
  delete r_;
}

cltl_formula *
binop::clone () const
{
  return new binop (type_, l_, r_);
}

std::string
binop::dump () const
{
  std::string res;
  res += "(";
  res += l_->dump ();
  res += ") ";
  switch (get_type ())
  {
    case AND:
      res += "&&";
      break;
    case OR:
      res += "||";
      break;
    case UNTIL:
      res += "U";
      break;
    case RELEASE:
      res += "R";
      break;
    case COST_UNTIL:
      res += "UN";
      break;
    case COST_RELEASE:
      res += "RN";
      break;
  }
  res += " (";
  res += r_->dump ();
  res += ")";
  return res;
}
