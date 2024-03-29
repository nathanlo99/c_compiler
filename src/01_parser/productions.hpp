
#pragma once

constexpr const char *const context_free_grammar =
    R"(#
procedures -> procedure procedures
procedures -> main
# Allow procedures to return any type
procedure -> type ID LPAREN params RPAREN LBRACE dcls statements RBRACE
main -> INT WAIN LPAREN dcl COMMA dcl RPAREN LBRACE dcls statements RBRACE
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
statement -> expr SEMI
statement -> IF LPAREN expr RPAREN LBRACE statements RBRACE ELSE LBRACE statements RBRACE
statement -> IF LPAREN expr RPAREN LBRACE statements RBRACE
statement -> FOR LPAREN expr SEMI expr SEMI expr RPAREN LBRACE statements RBRACE
statement -> WHILE LPAREN expr RPAREN LBRACE statements RBRACE
statement -> PRINTLN LPAREN expr RPAREN SEMI
statement -> DELETE LBRACK RBRACK expr SEMI
statement -> BREAK SEMI
statement -> CONTINUE SEMI
statement -> RETURN expr SEMI
# Exprs
# Precedence: 16 (right to left)
expr -> lvalue BECOMES expr
expr -> boolor
# Precedence: 15 (left to right)
boolor -> boolor BOOLOR booland
boolor -> booland
# Precedence: 14 (left to right)
booland -> booland BOOLAND eqtest
booland -> eqtest
# Precedence: 10 (left to right)
eqtest -> eqtest EQ test
eqtest -> eqtest NE test
eqtest -> test
# Precedence: 9 (left to right)
test -> test LT sum
test -> test LE sum
test -> test GE sum
test -> test GT sum
test -> sum
# Precedence: 6 (left to right)
sum -> term
sum -> sum PLUS term
sum -> sum MINUS term
# Precedence: 5 (left to right)
term -> factor
term -> term STAR factor
term -> term SLASH factor
term -> term PCT factor
# Precedence: 3 (right to left)
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
lvalue -> LPAREN lvalue RPAREN
#)";
