
#pragma once

constexpr const char *const context_free_grammar =
    R"(procedures -> procedure procedures
procedures -> main
# Allow procedures to return any type
procedure -> type ID LPAREN params RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE
main -> INT WAIN LPAREN dcl COMMA dcl RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE
params ->
params -> paramlist
paramlist -> dcl
paramlist -> dcl COMMA paramlist
type -> INT
type -> INT STAR
dcls ->
dcls -> dcls dcl BECOMES NUM SEMI
dcls -> dcls dcl BECOMES NULL SEMI
dcl -> type ID
# Statements
statements ->
statements -> statements statement
statement -> IF LPAREN test RPAREN LBRACE statements RBRACE ELSE LBRACE statements RBRACE
statement -> expr SEMI
statement -> IF LPAREN test RPAREN LBRACE statements RBRACE
statement -> FOR LPAREN expr SEMI expr SEMI expr RPAREN LBRACE statements RBRACE
statement -> WHILE LPAREN test RPAREN LBRACE statements RBRACE
statement -> PRINTLN LPAREN expr RPAREN SEMI
statement -> DELETE LBRACK RBRACK expr SEMI
# Exprs
# Precedence: 16
expr -> test
expr -> lvalue BECOMES expr
# Precedence: 9
test -> sum EQ sum
test -> sum NE sum
test -> sum LT sum
test -> sum LE sum
test -> sum GE sum
test -> sum GT sum
test -> sum
# Precedence: 6
sum -> term
sum -> sum PLUS term
sum -> sum MINUS term
# Precedence: 5
term -> factor
term -> term STAR factor
term -> term SLASH factor
term -> term PCT factor
# Precedence: 3
factor -> ID
factor -> NUM
factor -> NULL
factor -> LPAREN expr RPAREN
factor -> AMP lvalue
factor -> STAR factor
factor -> NEW INT LBRACK expr RBRACK
factor -> ID LPAREN RPAREN
factor -> ID LPAREN arglist RPAREN
arglist -> expr
arglist -> expr COMMA arglist
lvalue -> ID
lvalue -> STAR factor
lvalue -> LPAREN lvalue RPAREN)";
