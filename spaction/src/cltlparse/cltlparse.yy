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

%skeleton "lalr1.cc"    /* -*- use C++ -*- */

%{
#include <list>
#include <string>
#include "CltlFormula.h"
#include "CltlFormulaFactory.h"
#include "cltlparse/CLTLScanner.h"

// wrap a static default factory for CLTL formulae
static spaction::CltlFormulaFactory & _factory();

// A struct (instead of an union) to handle the various return types
struct union_tag {
    std::string apval;
    spaction::CltlFormulaPtr form;
    spaction::UnaryOperator::UnaryOperatorType u_type;
    spaction::BinaryOperator::BinaryOperatorType b_type;
};

// tell bison to use our custom struct for return values
#define YYSTYPE union_tag

// custom lex function
static int yylex(YYSTYPE*, spaction::cltlparse::CLTLScanner &);

// NOTE error handling is copied from SPOT LTL parser
/// \brief A parse diagnostic with its location.
typedef std::pair<yy::location, std::string> parse_error;
/// \brief A list of parser diagnostics, as filled by parse.
typedef std::list<parse_error> parse_error_list;

%}

%lex-param {spaction::cltlparse::CLTLScanner &scanner}

%parse-param {spaction::CltlFormulaPtr &result}
%parse-param {spaction::cltlparse::CLTLScanner &scanner}
%parse-param {parse_error_list &error_list}

/* token types */
%type   <form>                      formula
%type   <form>                      atomic
%type   <form>                      constant
%type   <u_type>                    unary
%type   <b_type>                    binary

%token                              END         0
%token                              LPAR
%token                              RPAR
%token                              TRUE
%token                              FALSE
%token                              NOT
%token                              NEXT
%token                              AND
%token                              OR
%token                              IMPLY
%token                              UNTIL
%token                              RELEASE
%token                              FINALLY
%token                              GLOBALLY
%token                              COSTUNTIL
%token                              COSTRELEASE
/*
%token                              COSTFINALLY
%token                              COSTGLOBALLY
*/
%token  <apval>                     ATOM

/* Priorities */

%right IMPLY
%left OR
%left AND

%right UNTIL RELEASE COSTUNTIL COSTRELEASE
%nonassoc FINALLY GLOBALLY
%nonassoc NEXT

%nonassoc NOT

%%

whole_formula: formula END { result = $1; };

formula
: atomic                    { $$ = $1; }
| constant                  { $$ = $1; }
| unary formula             { $$ = _factory().make_unary($1, $2); }
| formula IMPLY formula     { $$ = _factory().make_imply($1, $3); }
| formula binary formula    { $$ = _factory().make_binary($2, $1, $3); }
| LPAR formula RPAR         { $$ = $2; }
| FINALLY formula           { $$ = _factory().make_finally($2); }
| GLOBALLY formula          { $$ = _factory().make_globally($2); }
;

atomic: ATOM                { $$ = _factory().make_atomic($1); };

constant
: TRUE                      { $$ = _factory().make_constant(true); }
| FALSE                     { $$ = _factory().make_constant(false); }
;

unary
: NOT                       { $$ = spaction::UnaryOperator::kNot; }
| NEXT                      { $$ = spaction::UnaryOperator::kNext; }
;

binary
: AND                       { $$ = spaction::BinaryOperator::kAnd; }
| OR                        { $$ = spaction::BinaryOperator::kOr; }
| UNTIL                     { $$ = spaction::BinaryOperator::kUntil; }
| RELEASE                   { $$ = spaction::BinaryOperator::kRelease; }
| COSTUNTIL                 { $$ = spaction::BinaryOperator::kCostUntil; }
| COSTRELEASE               { $$ = spaction::BinaryOperator::kCostRelease; }
;

%%

void yy::parser::error(const location_type &location, const std::string &message) {
    error_list.push_back(parse_error(location, message));
}

spaction::CltlFormulaFactory & _factory() {
    static spaction::CltlFormulaFactory f = spaction::CltlFormulaFactory();
    return f;
}

int yylex(YYSTYPE *yylval, spaction::cltlparse::CLTLScanner &scanner) {
    return scanner.yylex(yylval);
}

#include <sstream>

#include "cltlparse/public.h"

namespace spaction {
namespace cltlparse {

CltlFormulaPtr parse_formula(const std::string &ltl_string) {
    CltlFormulaPtr f = nullptr;
    std::istringstream in = std::istringstream(ltl_string);
    parse_error_list error_list;
    CLTLScanner s(&in);
    yy::parser p(f, s, error_list);
    p.parse();

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
        //delete f;
        f = nullptr;
    }

    return f;
}

}  // namespace cltlparse
}  // namespace spaction


