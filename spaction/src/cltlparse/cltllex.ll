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

#include "cltlparse.hh"
#include "cltlparse/CLTLScanner.h"

typedef yy::parser::token token;

%}

%option c++
%option outfile="lex.yy.c"
%option noyywrap warn 8bit batch
%option yyclass="CLTLScanner"

%%

"("                 {   return token::LPAR; }
")"                 {   return token::RPAR; }

"true"              {   return token::TRUE; }
"false"             {   return token::FALSE; }

"&&"                {   return token::AND; }
"||"                {   return token::OR; }
"!"                 {   return token::NOT; }
"->"                {   return token::IMPLY; }

"F"                 {   return token::FINALLY; }
"G"                 {   return token::GLOBALLY; }
"U"                 {   return token::UNTIL; }
"R"                 {   return token::RELEASE; }
"X"                 {   return token::NEXT; }

"UN"                {   return token::COSTUNTIL; }
"RN"                {   return token::COSTRELEASE; }

"FN"                {   return token::COSTFINALLY; }
"GN"                {   return token::COSTGLOBALLY; }

\"[a-zA-Z0-9_\.]+\"    {   yylval->apval = yytext;
                        // remove double quotes around the atom
                        auto it = std::remove(yylval->apval.begin(), yylval->apval.end(), '"');
                        yylval->apval = std::string(yylval->apval.begin(), it);
                        return token::ATOM; }

%%
