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

#ifndef __spaction_atomic_h__
#define __spaction_atomic_h__

#include <string>
#include "cltl.h"

class atomic : public cltl_formula {
public:
  atomic (const std::string & d): data_ (d) {}
  ~atomic () {}
  
  atomic (const atomic &) = delete;
  atomic & operator= (const atomic &) = delete;
  
  cltl_formula * clone () const;
  
  std::string get () const { return data_; }
  
  void accept (cltl_visitor & v) const override
  { v.visit (this); }
  
  std::string dump () const;
private:
  std::string data_;
};

#endif // define __spaction_atomic_h__
