
#include "ast_node.hpp"

#include "parse_node.hpp"
#include "parser.hpp"
#include "util.hpp"
#include <memory>

int64_t parse_literal(const std::string &lexeme) {
  try {
    const int64_t result = std::stoll(lexeme, nullptr, 0);
    if (result < 0 && (lexeme.starts_with("0x") || lexeme.starts_with("0X")))
      return result + 0x1'0000'0000LL;
    return result;
  } catch (const std::exception &e) {
    throw std::runtime_error("Could not parse literal: " + lexeme);
  }
}

Type parse_node_to_type(const std::shared_ptr<ParseNode> &node) {
  const std::string production_str = node->production.to_string();
  debug_assert(node->production.product == "type",
               "Argument to parse_node_to_type was not derived from 'type'");
  if (production_str == "type -> INT")
    return Type::Int;
  if (production_str == "type -> INT STAR")
    return Type::IntStar;
  unreachable("Unknown type");
  return Type::Unknown;
}

Variable parse_node_to_variable(const std::shared_ptr<ParseNode> &node) {
  const auto type = parse_node_to_type(node->children[0]);
  const auto name = node->children[1]->token.lexeme;
  return Variable(name, type);
}

void check_reduce_functions(
    const std::unordered_map<std::string,
                             std::function<std::shared_ptr<ASTNode>(
                                 const std::shared_ptr<ParseNode> &)>>
        &reduce_functions) {
  const auto grammar = load_default_grammar();
  std::unordered_set<std::string> present_productions;
  std::unordered_set<std::string> grammar_productions;
  for (const auto &[production_str, func] : reduce_functions)
    present_productions.insert(production_str);
  for (const auto &[product, productions] : grammar.productions_by_product) {
    for (const auto &production : productions) {
      const std::string production_str = production.to_string();
      grammar_productions.insert(production_str);
    }
  }
  if (present_productions != grammar_productions) {
    std::unordered_set<std::string> missing_productions;
    std::unordered_set<std::string> extra_productions;
    std::set_difference(
        grammar_productions.begin(), grammar_productions.end(),
        present_productions.begin(), present_productions.end(),
        std::inserter(missing_productions, missing_productions.begin()));
    std::set_difference(
        present_productions.begin(), present_productions.end(),
        grammar_productions.begin(), grammar_productions.end(),
        std::inserter(extra_productions, extra_productions.begin()));
    std::string error_message = "Missing productions: \n";
    for (const auto &production : missing_productions)
      error_message += " - " + production + "\n";
    error_message += "\n";
    error_message += "Extra productions: \n";
    for (const auto &production : extra_productions)
      error_message += " - " + production + "\n";
    throw std::runtime_error(error_message);
  }
}

std::shared_ptr<ASTNode> construct_ast(const std::shared_ptr<ParseNode> &node) {
  const ContextFreeGrammar::Production production = node->production;
  const std::string production_str = production.to_string();

  using Func = std::function<std::shared_ptr<ASTNode>(
      const std::shared_ptr<ParseNode> &)>;
  const std::unordered_map<std::string, Func> reduce_functions = []() {
    std::unordered_map<std::string, Func> result;
    const auto &register_function = [&](const std::string &production_str,
                                        const Func &function) {
      if (result.count(production_str) > 0)
        throw std::runtime_error("Duplicate production: " + production_str);
      result[production_str] = function;
    };

    register_function(
        "procedures -> procedure procedures",
        [](const std::shared_ptr<ParseNode> &node) {
          auto procedure = construct_ast<Procedure>(node->children[0]);
          auto program = construct_ast<Program>(node->children[1]);
          program->procedures.insert(program->procedures.begin(), *procedure);
          return program;
        });

    register_function(
        "procedures -> main", [](const std::shared_ptr<ParseNode> &node) {
          auto program = std::make_shared<Program>();
          auto main_procedure = construct_ast<Procedure>(node->children[0]);
          program->procedures.push_back(*main_procedure);
          return program;
        });

    register_function(
        "procedure -> type ID LPAREN params RPAREN LBRACE dcls statements "
        "RETURN expr SEMI RBRACE",
        [](const std::shared_ptr<ParseNode> &node) {
          const auto procedure_name = node->children[1]->token.lexeme;
          const auto return_type = parse_node_to_type(node->children[0]);
          const auto params = construct_ast<ParameterList>(node->children[3]);
          const auto decls = construct_ast<DeclarationList>(node->children[6]);
          const auto statements = construct_ast<Statements>(node->children[7]);
          const auto return_expr = construct_ast<Expr>(node->children[9]);

          return std::make_shared<Procedure>(procedure_name, params,
                                             return_type, decls, statements,
                                             return_expr);
        });

    register_function(
        "main -> INT WAIN LPAREN dcl COMMA dcl RPAREN LBRACE dcls "
        "statements RETURN expr SEMI RBRACE",
        [](const std::shared_ptr<ParseNode> &node) {
          const std::string procedure_name = "wain";
          const auto return_type = Type::Int;
          const auto first_variable = parse_node_to_variable(node->children[3]);
          const auto second_variable =
              parse_node_to_variable(node->children[5]);

          const auto params = std::make_shared<ParameterList>(
              std::vector<Variable>({first_variable, second_variable}));

          const auto decls = construct_ast<DeclarationList>(node->children[8]);
          const auto statements = construct_ast<Statements>(node->children[9]);
          const auto return_expr = construct_ast<Expr>(node->children[11]);

          return std::make_shared<Procedure>(procedure_name, params,
                                             return_type, decls, statements,
                                             return_expr);
        });

    register_function("params ->", [](const std::shared_ptr<ParseNode> &) {
      return std::make_shared<ParameterList>();
    });

    register_function("params -> paramlist",
                      [](const std::shared_ptr<ParseNode> &node) {
                        return construct_ast<ParameterList>(node->children[0]);
                      });

    register_function(
        "paramlist -> dcl", [](const std::shared_ptr<ParseNode> &node) {
          const auto decl = parse_node_to_variable(node->children[0]);
          return std::make_shared<ParameterList>(std::vector<Variable>{decl});
        });

    register_function(
        "paramlist -> dcl COMMA paramlist",
        [](const std::shared_ptr<ParseNode> &node) {
          const auto first = parse_node_to_variable(node->children[0]);
          auto rest = construct_ast<ParameterList>(node->children[2]);
          rest->parameters.insert(rest->parameters.begin(), first);
          return rest;
        });

    register_function("dcls ->", [](const std::shared_ptr<ParseNode> &) {
      return std::make_shared<DeclarationList>();
    });

    register_function("dcls -> dcls dcl BECOMES NUM SEMI",
                      [](const std::shared_ptr<ParseNode> &node) {
                        auto rest =
                            construct_ast<DeclarationList>(node->children[0]);
                        auto decl = parse_node_to_variable(node->children[1]);
                        const int64_t value =
                            parse_literal(node->children[3]->token.lexeme);
                        decl.initial_value = Literal(value, decl.type);
                        rest->declarations.push_back(decl);
                        return rest;
                      });

    register_function("dcls -> dcls dcl BECOMES NULL SEMI",
                      [](const std::shared_ptr<ParseNode> &node) {
                        auto rest =
                            construct_ast<DeclarationList>(node->children[0]);
                        auto decl = parse_node_to_variable(node->children[1]);
                        decl.initial_value = Literal::null();
                        rest->declarations.push_back(decl);
                        return rest;
                      });

    register_function("statements ->", [](const std::shared_ptr<ParseNode> &) {
      return std::make_shared<Statements>();
    });

    register_function("statements -> statements statement",
                      [](const std::shared_ptr<ParseNode> &node) {
                        auto rest =
                            construct_ast<Statements>(node->children[0]);
                        const auto statement =
                            construct_ast<Statement>(node->children[1]);
                        rest->statements.push_back(statement);
                        return rest;
                      });
    register_function("statement -> lvalue BECOMES expr SEMI",
                      [](const std::shared_ptr<ParseNode> &node) {
                        const auto lhs =
                            construct_ast<LValueExpr>(node->children[0]);
                        const auto rhs = construct_ast<Expr>(node->children[2]);
                        const auto expr =
                            std::make_shared<AssignmentExpr>(lhs, rhs);
                        return std::make_shared<ExprStatement>(expr);
                      });

    register_function("statement -> IF LPAREN test RPAREN LBRACE "
                      "statements RBRACE ELSE LBRACE statements RBRACE",
                      [](const std::shared_ptr<ParseNode> &node) {
                        const auto test =
                            construct_ast<Expr>(node->children[2]);
                        const auto true_statements =
                            construct_ast<Statements>(node->children[5]);
                        const auto false_statements =
                            construct_ast<Statements>(node->children[9]);
                        return std::make_shared<IfStatement>(
                            test, *true_statements, *false_statements);
                      });

    register_function(
        "statement -> IF LPAREN test RPAREN LBRACE "
        "statements RBRACE",
        [](const std::shared_ptr<ParseNode> &node) {
          const auto test = construct_ast<Expr>(node->children[2]);
          const auto true_statements =
              construct_ast<Statements>(node->children[5]);
          const auto false_statements = std::make_shared<Statements>();
          return std::make_shared<IfStatement>(test, *true_statements,
                                               *false_statements);
        });

    register_function(
        "statement -> WHILE LPAREN test RPAREN LBRACE statements RBRACE",
        [](const std::shared_ptr<ParseNode> &node) {
          const auto test = construct_ast<Expr>(node->children[2]);
          const auto body_statement =
              construct_ast<Statements>(node->children[5]);
          return std::make_shared<WhileStatement>(test, body_statement);
        });

    register_function("statement -> PRINTLN LPAREN expr RPAREN SEMI",
                      [](const std::shared_ptr<ParseNode> &node) {
                        const auto expr =
                            construct_ast<Expr>(node->children[2]);
                        return std::make_shared<PrintStatement>(expr);
                      });

    register_function("statement -> DELETE LBRACK RBRACK expr SEMI",
                      [](const std::shared_ptr<ParseNode> &node) {
                        const auto expr =
                            construct_ast<Expr>(node->children[3]);
                        return std::make_shared<DeleteStatement>(expr);
                      });

    const auto make_test_expr = [](const std::shared_ptr<ParseNode> &node) {
      const auto lhs = construct_ast<Expr>(node->children[0]);
      const auto op = node->children[1]->token;
      const auto rhs = construct_ast<Expr>(node->children[2]);
      return std::make_shared<TestExpr>(
          lhs, token_to_comparison_operation(op.kind), rhs);
    };
    const auto test_productions = {
        "test -> expr EQ expr", "test -> expr NE expr", "test -> expr LT expr",
        "test -> expr LE expr", "test -> expr GE expr", "test -> expr GT expr"};
    for (const auto &test_production : test_productions)
      register_function(test_production, make_test_expr);

    const auto make_binary_expr = [](const std::shared_ptr<ParseNode> &node) {
      const auto lhs = construct_ast<Expr>(node->children[0]);
      const auto op = node->children[1]->token;
      const auto rhs = construct_ast<Expr>(node->children[2]);
      return std::make_shared<BinaryExpr>(
          lhs, token_to_binary_operation(op.kind), rhs);
    };
    const auto binary_productions = {
        "expr -> expr PLUS term", "expr -> expr MINUS term",
        "term -> term STAR factor", "term -> term SLASH factor",
        "term -> term PCT factor"};
    for (const auto &binary_production : binary_productions)
      register_function(binary_production, make_binary_expr);

    return result;
  }();

  // check_reduce_functions(reduce_functions);

  if (reduce_functions.count(production_str) > 0)
    return reduce_functions.at(production_str)(node);

  if (production_str == "expr -> term") {
    return construct_ast<Expr>(node->children[0]);
  } else if (production_str == "term -> factor") {
    return construct_ast<Expr>(node->children[0]);
  } else if (production_str == "factor -> ID") {
    const auto variable_name = node->children[0]->token.lexeme;
    const auto variable = Variable(variable_name, Type::Unknown);
    return std::make_shared<VariableExpr>(variable);
  } else if (production_str == "factor -> NUM") {
    const auto value = std::stoi(node->children[0]->token.lexeme);
    return std::make_shared<LiteralExpr>(Literal(value, Type::Int));
  } else if (production_str == "factor -> NULL") {
    return std::make_shared<LiteralExpr>(Literal::null());
  } else if (production_str == "factor -> LPAREN expr RPAREN") {
    return construct_ast<Expr>(node->children[1]);
  } else if (production_str == "factor -> AMP lvalue") {
    const auto rhs = construct_ast<LValueExpr>(node->children[1]);
    // &(*expr) == expr
    if (const auto dereference_node =
            std::dynamic_pointer_cast<DereferenceLValueExpr>(rhs)) {
      return dereference_node->argument;
    } else if (const auto variable_node =
                   std::dynamic_pointer_cast<VariableLValueExpr>(rhs)) {
      return std::make_shared<AddressOfExpr>(variable_node);
    } else {
      debug_assert(false, "lvalue argument to address-of operator was "
                          "neither dereference nor variable");
    }
  } else if (production_str == "factor -> STAR factor") {
    const auto rhs = construct_ast<Expr>(node->children[1]);
    // *(&value) == value1, where [value] on the left is an lvalue, and [value1]
    // is the associated rvalue
    if (auto address_of_expr = std::dynamic_pointer_cast<AddressOfExpr>(rhs)) {
      if (auto variable_expr = std::dynamic_pointer_cast<VariableLValueExpr>(
              address_of_expr->argument)) {
        return std::make_shared<VariableExpr>(variable_expr->variable);
      }
    }
    return std::make_shared<DereferenceExpr>(rhs);
  } else if (production_str == "factor -> NEW INT LBRACK expr RBRACK") {
    const auto rhs = construct_ast<Expr>(node->children[3]);
    return std::make_shared<NewExpr>(rhs);
  } else if (production_str == "factor -> ID LPAREN RPAREN") {
    const std::string procedure_name = node->children[0]->token.lexeme;
    return std::make_shared<FunctionCallExpr>(procedure_name);
  } else if (production_str == "factor -> ID LPAREN arglist RPAREN") {
    const std::string procedure_name = node->children[0]->token.lexeme;
    const auto arguments = construct_ast<ArgumentList>(node->children[2]);
    return std::make_shared<FunctionCallExpr>(procedure_name, arguments->exprs);
  } else if (production_str == "arglist -> expr") {
    const auto expr = construct_ast<Expr>(node->children[0]);
    return std::make_shared<ArgumentList>(
        std::vector<std::shared_ptr<Expr>>{expr});
  } else if (production_str == "arglist -> expr COMMA arglist") {
    const auto expr = construct_ast<Expr>(node->children[0]);
    auto rest = construct_ast<ArgumentList>(node->children[2]);
    rest->exprs.insert(rest->exprs.begin(), expr);
    return rest;
  } else if (production_str == "lvalue -> ID") {
    const std::string variable_name = node->children[0]->token.lexeme;
    const Variable variable(variable_name, Type::Unknown);
    return std::make_shared<VariableLValueExpr>(variable);
  } else if (production_str == "lvalue -> STAR factor") {
    const auto rhs = construct_ast<Expr>(node->children[1]);
    // *(&value) == value, as lvalues
    if (auto address_of_expr = std::dynamic_pointer_cast<AddressOfExpr>(rhs)) {
      return address_of_expr->argument;
    }
    return std::make_shared<DereferenceLValueExpr>(rhs);
  } else if (production_str == "lvalue -> LPAREN lvalue RPAREN") {
    return construct_ast<LValueExpr>(node->children[1]);
  }
  // Begin augmented productions
  else if (production_str == "expr -> test") {
    return construct_ast<Expr>(node->children[0]);
  } else if (production_str == "test -> sum GE sum" ||
             production_str == "test -> sum GT sum" ||
             production_str == "test -> sum LE sum" ||
             production_str == "test -> sum LT sum" ||
             production_str == "test -> sum NE sum" ||
             production_str == "test -> sum EQ sum") {
    const auto lhs = construct_ast<Expr>(node->children[0]);
    const auto op = node->children[1]->token.kind;
    const auto rhs = construct_ast<Expr>(node->children[2]);
    return std::make_shared<TestExpr>(lhs, token_to_comparison_operation(op),
                                      rhs);
  } else if (production_str == "test -> sum") {
    return construct_ast<Expr>(node->children[0]);
  } else if (production_str == "sum -> term") {
    return construct_ast<Expr>(node->children[0]);
  } else if (production_str == "sum -> sum PLUS term" ||
             production_str == "sum -> sum MINUS term") {
    const auto lhs = construct_ast<Expr>(node->children[0]);
    const auto op = node->children[1]->token.kind;
    const auto rhs = construct_ast<Expr>(node->children[2]);
    return std::make_shared<BinaryExpr>(lhs, token_to_binary_operation(op),
                                        rhs);
  } else if (production_str == "statement -> expr SEMI") {
    return std::make_shared<ExprStatement>(
        construct_ast<Expr>(node->children[0]));
  } else if (production_str == "expr -> lvalue BECOMES expr") {
    const auto lhs = construct_ast<LValueExpr>(node->children[0]);
    const auto rhs = construct_ast<Expr>(node->children[2]);
    return std::make_shared<AssignmentExpr>(lhs, rhs);
  } else if (production_str == "statement -> FOR LPAREN expr SEMI expr SEMI "
                               "expr RPAREN LBRACE statements RBRACE") {
    const auto init = construct_ast<Expr>(node->children[2]);
    const auto cond = construct_ast<Expr>(node->children[4]);
    const auto update = construct_ast<Expr>(node->children[6]);
    auto body = construct_ast<Statements>(node->children[9]);

    // for (init; cond; update) { body; }
    //   BECOMES
    // init; while (cond) { body; update; }
    auto result = std::make_shared<Statements>();
    result->statements.push_back(std::make_shared<ExprStatement>(init));

    body->statements.push_back(std::make_shared<ExprStatement>(update));
    auto while_loop = std::make_shared<WhileStatement>(cond, body);

    result->statements.push_back(while_loop);
    return result;
  }
  unreachable("WARN: Production '" + production_str + "' not yet handled");

  return nullptr;
}
