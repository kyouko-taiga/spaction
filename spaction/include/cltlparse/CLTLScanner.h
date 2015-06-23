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

#ifndef SPACTION_INCLUDE_CLTLPARSE_CLTLSCANNER_H_
#define SPACTION_INCLUDE_CLTLPARSE_CLTLSCANNER_H_

#if ! defined(yyFlexLexerOnce)
#include <FlexLexer.h>
#endif

#undef YY_DECL
#define YY_DECL int spaction::cltlparse::CLTLScanner::yylex()

#include "cltlparse.hh"

struct union_tag;

namespace spaction {
namespace cltlparse {

class CLTLScanner : public yyFlexLexer {
 public:
    explicit CLTLScanner(std::istream *in): yyFlexLexer(in) {}

    int yylex(struct union_tag *lval) {
        yylval = lval;
        return yylex();
    }

 private:
    struct union_tag *yylval;
    int yylex();
};

}  // namespace cltlparse
}  // namespace spaction

#endif  // SPACTION_INCLUDE_CLTLPARSE_CLTLSCANNER_H_
