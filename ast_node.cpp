
#include "ast_node.hpp"

#include "parse_node.hpp"
#include "util.hpp"
#include <memory>

Type parse_node_to_type(std::shared_ptr<ParseNode> node) {
  const std::string production_str = node->production.to_string();
  runtime_assert(node->production.product == "type",
                 "Argument to parse_node_to_type was not derived from 'type'");
  if (production_str == "type -> INT")
    return Type::Int;
  if (production_str == "type -> INT STAR")
    return Type::IntStar;
  unreachable("Unknown type");
  return Type::Unknown;
}

Variable parse_node_to_variable(std::shared_ptr<ParseNode> node) {
  const auto type = parse_node_to_type(node->children[0]);
  const auto name = node->children[1]->token.lexeme;
  return Variable(name, type, Literal());
}

std::shared_ptr<ASTNode> parse_tree_to_ast(std::shared_ptr<ParseNode> node) {
  const CFG::Production production = node->production;
  const std::string production_str = production.to_string();

  if (production_str == "start -> BOF procedures EOF") {
    return parse_tree_to_ast(node->children[1]);
  } else if (production_str == "procedures -> procedure procedures") {
    auto procedure = convert<Procedure>(parse_tree_to_ast(node->children[0]));
    auto program = convert<Program>(parse_tree_to_ast(node->children[1]));
    program->procedures.insert(program->procedures.begin(), *procedure);
    return program;
  } else if (production_str == "procedures -> main") {
    auto program = std::make_shared<Program>();
    auto main_procedure =
        convert<Procedure>(parse_tree_to_ast(node->children[0]));
    program->procedures.push_back(*main_procedure);
    return program;
  } else if (production_str ==
             "procedure -> INT ID LPAREN params RPAREN LBRACE dcls statements "
             "RETURN expr SEMI RBRACE") {
    const auto procedure_name = node->children[1]->token.lexeme;
    const auto return_type = Type::Int;
    const auto params =
        convert<ParameterList>(parse_tree_to_ast(node->children[3]));
    const auto decls =
        convert<DeclarationList>(parse_tree_to_ast(node->children[6]));
    const auto statements =
        convert<Statements>(parse_tree_to_ast(node->children[7]));
    const auto return_expr =
        convert<Expr>(parse_tree_to_ast(node->children[9]));

    return std::make_shared<Procedure>(procedure_name, params, return_type,
                                       decls, statements, return_expr);
  } else if (production_str ==
             "main -> INT WAIN LPAREN dcl COMMA dcl RPAREN LBRACE dcls "
             "statements RETURN expr SEMI RBRACE") {
    const std::string procedure_name = "wain";
    const auto return_type = Type::Int;
    const auto first_variable = parse_node_to_variable(node->children[3]);
    const auto second_variable = parse_node_to_variable(node->children[5]);

    const auto params = std::make_shared<ParameterList>(
        std::vector<Variable>({first_variable, second_variable}));

    const auto decls =
        convert<DeclarationList>(parse_tree_to_ast(node->children[8]));
    const auto statements =
        convert<Statements>(parse_tree_to_ast(node->children[9]));
    const auto return_expr =
        convert<Expr>(parse_tree_to_ast(node->children[11]));

    return std::make_shared<Procedure>(procedure_name, params, return_type,
                                       decls, statements, return_expr);
  } else if (production_str == "params ->") {
    return std::make_shared<ParameterList>();
  } else if (production_str == "params -> paramlist") {
    return parse_tree_to_ast(node->children[0]);
  } else if (production_str == "paramlist -> dcl") {
    const auto decl = parse_node_to_variable(node->children[0]);
    return std::make_shared<ParameterList>(std::vector<Variable>{decl});
  } else if (production_str == "paramlist -> dcl COMMA paramlist") {
    const auto first = parse_node_to_variable(node->children[0]);
    auto rest = convert<ParameterList>(parse_tree_to_ast(node->children[2]));
    rest->parameters.insert(rest->parameters.begin(), first);
    return rest;
  } else if (production_str == "type -> INT") {
    unreachable("Handled further up");
  } else if (production_str == "type -> INT STAR") {
    unreachable("Handled further up");
  } else if (production_str == "dcls ->") {
    return std::make_shared<DeclarationList>();
  } else if (production_str == "dcls -> dcls dcl BECOMES NUM SEMI") {
    auto rest = convert<DeclarationList>(parse_tree_to_ast(node->children[0]));
    auto decl = parse_node_to_variable(node->children[1]);
    decl.initial_value =
        Literal(std::stoi(node->children[3]->token.lexeme), Type::Int);
    rest->declarations.push_back(decl);
    return rest;
  } else if (production_str == "dcls -> dcls dcl BECOMES NULL SEMI") {
    auto rest = convert<DeclarationList>(parse_tree_to_ast(node->children[0]));
    auto decl = parse_node_to_variable(node->children[1]);
    decl.initial_value = Literal(0, Type::IntStar);
    rest->declarations.push_back(decl);
    return rest;
  } else if (production_str == "dcl -> type ID") {
    unreachable(production_str + ": should be handled further up");
  } else if (production_str == "statements ->") {
    return std::make_shared<Statements>();
  } else if (production_str == "statements -> statements statement") {
    auto rest = convert<Statements>(parse_tree_to_ast(node->children[0]));
    const auto statement =
        convert<Statement>(parse_tree_to_ast(node->children[1]));
    rest->statements.push_back(*statement);
    return rest;
  } else if (production_str == "statement -> lvalue BECOMES expr SEMI") {
    const auto lhs = convert<LValueExpr>(parse_tree_to_ast(node->children[0]));
    const auto rhs = convert<Expr>(parse_tree_to_ast(node->children[2]));
    return std::make_shared<AssignmentStatement>(lhs, rhs);
  } else if (production_str ==
             "statement -> IF LPAREN test RPAREN LBRACE "
             "statements RBRACE ELSE LBRACE statements RBRACE") {
    const auto test = convert<TestExpr>(parse_tree_to_ast(node->children[2]));
    const auto true_statements =
        convert<Statements>(parse_tree_to_ast(node->children[5]));
    const auto false_statements =
        convert<Statements>(parse_tree_to_ast(node->children[9]));
    return std::make_shared<IfStatement>(test, true_statements,
                                         false_statements);
  } else if (production_str ==
             "statement -> WHILE LPAREN test RPAREN LBRACE statements RBRACE") {
    const auto test = convert<TestExpr>(parse_tree_to_ast(node->children[2]));
    const auto body_statement =
        convert<Statements>(parse_tree_to_ast(node->children[5]));
    return std::make_shared<WhileStatement>(test, body_statement);
  } else if (production_str == "statement -> PRINTLN LPAREN expr RPAREN SEMI") {
    const auto expr = convert<Expr>(parse_tree_to_ast(node->children[2]));
    return std::make_shared<PrintStatement>(expr);
  } else if (production_str == "statement -> DELETE LBRACK RBRACK expr SEMI") {
    const auto expr = convert<Expr>(parse_tree_to_ast(node->children[3]));
    return std::make_shared<DeleteStatement>(expr);
  } else if (production_str == "test -> expr EQ expr" ||
             production_str == "test -> expr NE expr" ||
             production_str == "test -> expr LT expr" ||
             production_str == "test -> expr LE expr" ||
             production_str == "test -> expr GE expr" ||
             production_str == "test -> expr GT expr" ||
             production_str == "expr -> expr PLUS term" ||
             production_str == "expr -> expr MINUS term" ||
             production_str == "term -> term STAR factor" ||
             production_str == "term -> term SLASH factor" ||
             production_str == "term -> term PCT factor") {
    const auto lhs = convert<Expr>(parse_tree_to_ast(node->children[0]));
    const auto op = node->children[1]->token;
    const auto rhs = convert<Expr>(parse_tree_to_ast(node->children[2]));
    return std::make_shared<BinaryExpr>(lhs, op, rhs);
  } else if (production_str == "expr -> term") {
    return parse_tree_to_ast(node->children[0]);
  } else if (production_str == "term -> factor") {
    return parse_tree_to_ast(node->children[0]);
  } else if (production_str == "factor -> ID") {
    const auto variable_name = node->children[0]->token.lexeme;
    const auto variable = Variable(variable_name, Type::Unknown, Literal());
    return std::make_shared<VariableExpr>(variable);
  } else if (production_str == "factor -> NUM") {
    const auto value = std::stoi(node->children[0]->token.lexeme);
    return std::make_shared<LiteralExpr>(Literal(value, Type::Int));
  } else if (production_str == "factor -> NULL") {
    return std::make_shared<LiteralExpr>(Literal(0, Type::IntStar));
  } else if (production_str == "factor -> LPAREN expr RPAREN") {
    return parse_tree_to_ast(node->children[1]);
  } else if (production_str == "factor -> AMP lvalue") {
    // TODO
  } else if (production_str == "factor -> STAR factor") {
    // TODO
  } else if (production_str == "factor -> NEW INT LBRACK expr RBRACK") {
    // TODO
  } else if (production_str == "factor -> ID LPAREN RPAREN") {
    // TODO
  } else if (production_str == "factor -> ID LPAREN arglist RPAREN") {
    // TODO
  } else if (production_str == "arglist -> expr") {
    // TODO
  } else if (production_str == "arglist -> expr COMMA arglist") {
    // TODO
  } else if (production_str == "lvalue -> ID") {
    // TODO
  } else if (production_str == "lvalue -> STAR factor") {
    // TODO
  } else if (production_str == "lvalue -> LPAREN lvalue RPAREN") {
    return parse_tree_to_ast(node->children[1]);
  }
  std::cerr << "WARN: Production " << production_str << " not yet handled"
            << std::endl;

  return nullptr;
}
