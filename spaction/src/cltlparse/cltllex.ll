%{
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

#include <algorithm>

#include "cltlparse.hh"
#include "cltlparse/CLTLScanner.h"

typedef yy::parser::token token;

// The location of the current token.
static yy::location loc;

%}

%option outfile="cltllex.cc"
%option noyywrap warn 8bit batch

%{
// Code run each time a pattern is matched.
#define YY_USER_ACTION  loc.columns (yyleng);
%}

%%

%{
// Code run each time yylex is called.
loc.step ();
%}

"("                 {   return yy::parser::make_LPAR(loc); }
")"                 {   return yy::parser::make_RPAR(loc); }

"true"              {   return yy::parser::make_TRUE(loc); }
"false"             {   return yy::parser::make_FALSE(loc); }

"&&"                {   return yy::parser::make_AND(loc); }
"||"                {   return yy::parser::make_OR(loc); }
"!"                 {   return yy::parser::make_NOT(loc); }
"->"                {   return yy::parser::make_IMPLY(loc); }

"F"                 {   return yy::parser::make_FINALLY(loc); }
"G"                 {   return yy::parser::make_GLOBALLY(loc); }
"U"                 {   return yy::parser::make_UNTIL(loc); }
"R"                 {   return yy::parser::make_RELEASE(loc); }
"X"                 {   return yy::parser::make_NEXT(loc); }

"UN"                {   return yy::parser::make_COSTUNTIL(loc); }
"RN"                {   return yy::parser::make_COSTRELEASE(loc); }

"FN"                {   return yy::parser::make_COSTFINALLY(loc); }
"GN"                {   return yy::parser::make_COSTGLOBALLY(loc); }

\"[a-zA-Z0-9_\.]+\" {   std::string tmp = yytext;
                        // remove double quotes around the atom
                        auto it = std::remove(tmp.begin(), tmp.end(), '"');
                        tmp = std::string(tmp.begin(), it);
                        return yy::parser::make_ATOM(tmp, loc); }

<<EOF>>             {   return yy::parser::make_END(loc); }

%%

#include "cltlparse/public.h"

namespace spaction {
namespace cltlparse {

CltlFormulaPtr parse_formula(const std::string &ltl_string) {
    CltlFormulaPtr f = nullptr;
    parse_error_list error_list;
    {
        auto yy_buffer = yy_scan_string(ltl_string.c_str());
        yy::parser p(f, error_list);
        p.parse();
        yy_delete_buffer(yy_buffer);
    }

    // code copied from SPOT
    bool printed = false;
    parse_error_list::const_iterator it;
    for (it = error_list.begin(); it != error_list.end(); ++it)
    {
        std::cerr << ">>> " << ltl_string << std::endl;
        const yy::location& l = it->first;

        unsigned n = 1;
        for (; n < 4 + l.begin.column; ++n)
            std::cerr << ' ';
        // Write at least one '^', even if begin==end.
        std::cerr << '^';
        ++n;
        for (; n < 4 + l.end.column; ++n)
            std::cerr << '^';
        std::cerr << std::endl << it->second << std::endl << std::endl;
        printed = true;
    }
    // end code from SPOT

    if (printed) {
        f = nullptr;
    }

    return f;
}

}  // namespace cltlparse
}  // namespace spaction
