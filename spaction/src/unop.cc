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

#include "unop.h"

unop::unop (unop_type t, const cltl_formula * s)
: type_ (t)
, son_ (s->clone ())
{}

unop::~unop ()
{
  delete son_;
}

cltl_formula *
unop::clone () const
{
  return new unop (type_, son_);
}

std::string
unop::dump () const
{
  std::string res;
  switch (type_)
  {
    case NEXT:
      res = "X";
      break;
    case NOT:
      res = "!";
      break;
  }
  res += " (";
  res += son_->dump ();
  res += ")";
  return res;
}
