
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <utility>
#include <vector>

#include "util.hpp"

enum Opcode {
  Add,
  Sub,
  Mult,
  Multu,
  Div,
  Divu,
  Mfhi,
  Mflo,
  Lis,
  Lw,
  Sw,
  Slt,
  Sltu,
  Beq,
  Bne,
  Jr,
  Jalr,
  Word,
  Label,
  Import,
  Comment,
  NumOpcodes,
};

struct Instruction {
  Opcode opcode;
  int s, t, d;
  int32_t i;
  std::string label_name;
  bool has_label;
  std::string comment_value;

  static constexpr std::array<const char *, NumOpcodes> opcode_to_string = {
      "add",  "sub", "mult", "multu", "div",     "divu",  "mfhi",
      "mflo", "lis", "lw",   "sw",    "slt",     "sltu",  "beq",
      "bne",  "jr",  "jalr", "word",  "[label]", "[raw]", "[comment]"};

private:
  Instruction(Opcode opcode, int s, int t, int d, int32_t i,
              bool has_label = false, const std::string &label_name = "")
      : opcode(opcode), s(s), t(t), d(d), i(i), has_label(has_label),
        label_name(label_name) {}
  Instruction(const std::string &label_name)
      : opcode(Label), s(0), t(0), d(0), i(0), label_name(label_name),
        has_label(true) {}

public:
  static Instruction add(int d, int s, int t) {
    return Instruction(Add, s, t, d, 0);
  }
  static Instruction sub(int d, int s, int t) {
    return Instruction(Sub, s, t, d, 0);
  }
  static Instruction mult(int s, int t) {
    return Instruction(Mult, s, t, 0, 0);
  }
  static Instruction multu(int s, int t) {
    return Instruction(Multu, s, t, 0, 0);
  }
  static Instruction div(int s, int t) { return Instruction(Div, s, t, 0, 0); }
  static Instruction divu(int s, int t) {
    return Instruction(Divu, s, t, 0, 0);
  }
  static Instruction mfhi(int d) { return Instruction(Mfhi, 0, 0, d, 0); }
  static Instruction mflo(int d) { return Instruction(Mflo, 0, 0, d, 0); }
  static Instruction lis(int d) { return Instruction(Lis, 0, 0, d, 0); }
  static Instruction lw(int t, int32_t i, int s) {
    return Instruction(Lw, s, t, 0, i);
  }
  static Instruction sw(int t, int32_t i, int s) {
    return Instruction(Sw, s, t, 0, i);
  }
  static Instruction slt(int d, int s, int t) {
    return Instruction(Slt, s, t, d, 0);
  }
  static Instruction sltu(int d, int s, int t) {
    return Instruction(Sltu, s, t, d, 0);
  }
  static Instruction beq(int s, int t, int i) {
    return Instruction(Beq, s, t, 0, i);
  }
  static Instruction beq(int s, int t, const std::string &label) {
    return Instruction(Beq, s, t, 0, 0, true, label);
  }
  static Instruction bne(int s, int t, int i) {
    return Instruction(Bne, s, t, 0, i);
  }
  static Instruction bne(int s, int t, const std::string &label) {
    return Instruction(Bne, s, t, 0, 0, true, label);
  }

  static Instruction jr(int s) { return Instruction(Jr, s, 0, 0, 0); }
  static Instruction jalr(int s) { return Instruction(Jalr, s, 0, 0, 0); }
  static Instruction word(int32_t i) { return Instruction(Word, 0, 0, 0, i); }
  static Instruction word(const std::string &label) {
    return Instruction(Word, 0, 0, 0, 0, true, label);
  }
  static Instruction label(const std::string &name) {
    return Instruction(name);
  }
  static Instruction import(const std::string &value) {
    return Instruction(Import, 0, 0, 0, 0, true, value);
  }
  static Instruction comment(const std::string &value) {
    return Instruction(Comment, 0, 0, 0, 0, true, value);
  }

  std::string to_string() const {
    std::stringstream ss;
    const std::string name = opcode_to_string.at(opcode);
    switch (opcode) {
    case Add:
    case Sub:
    case Slt:
    case Sltu:
      ss << name << " $" << d << ", $" << s << ", $" << t;
      break;
    case Mult:
    case Multu:
    case Div:
    case Divu:
      ss << name << " $" << s << ", $" << t;
      break;
    case Mfhi:
    case Mflo:
    case Lis:
      ss << name << " $" << d;
      break;
    case Lw:
    case Sw:
      ss << name << " $" << t << ", " << i << "($" << s << ")";
      break;
    case Beq:
    case Bne:
      if (has_label) {
        ss << name << " $" << s << ", $" << t << ", " << label_name;
      } else {
        ss << name << " $" << s << ", $" << t << ", " << i;
      }
      break;
    case Jr:
    case Jalr:
      ss << name << " $" << s;
      break;
    case Word:
      if (has_label) {
        ss << ".word " << label_name;
      } else {
        ss << ".word " << i;
      }
      break;
    case Label:
      ss << label_name << ":";
      break;
    case Import:
      ss << ".import " << label_name;
      break;
    case Comment:
      ss << "; " << label_name;
      break;
    default:
      runtime_assert(false, "Invalid opcode " + std::to_string(opcode));
    }

    const int instruction_width = 20;
    const int padding = std::max(0, instruction_width - (int)ss.str().size());
    if (comment_value != "")
      ss << std::string(padding, ' ') << "; " << comment_value;
    return ss.str();
  }
};

enum class Type {
  Void,
  Int,
  IntStar,
};

std::string type_to_string(const Type type) {
  switch (type) {
  case Type::Void:
    return "";
  case Type::Int:
    return "int";
  case Type::IntStar:
    return "int*";
  }
}

Type string_to_type(const std::string &string) {
  if (string == "type INT")
    return Type::Int;
  else if (string == "type INT STAR")
    return Type::IntStar;
  runtime_assert(false, "Invalid type string: " + string);
  return Type::Void;
}

struct ParseNode {
  std::string name;
  std::vector<std::shared_ptr<ParseNode>> children;
  std::string lexeme;
  std::string production;
  Type type = Void;

  ParseNode(const std::string &name) : name(name) {}

  void debug(int depth = 0) const {
    std::cout << std::string(2 * depth, ' ') << name;
    if (type != Type::Void)
      std::cout << ": " << type_to_string(type);
    std::cout << std::endl;
    for (const auto &child : children)
      child->debug(depth + 1);
  }

  void print() const {
    std::cout << production;
    if (type != Type::Void)
      std::cout << " : " << type_to_string(type);
    std::cout << std::endl;
    for (const auto &child : children)
      child->print();
  }

  void get_wlp_code(std::ostream &os) const {
    if (lexeme != "")
      os << lexeme << " ";
    for (const auto &child : children)
      child->get_wlp_code(os);
  }

  std::string wlp_code() const {
    std::stringstream ss;
    get_wlp_code(ss);
    return ss.str();
  }
};

struct ProcedureTable {
  std::string name;
  std::map<std::string, Type> symbol_table;
  std::set<std::string> used_variables;
  std::map<std::string, int> offsets;
  int next_offset;

  Type return_type;
  std::vector<Type> argument_types;

  void add_variable(const std::string &variable_name, const Type type) {
    if (symbol_table.count(variable_name) > 0)
      throw std::runtime_error("Duplicate symbol " + variable_name +
                               " in procedure " + name);

    symbol_table[variable_name] = type;
    offsets[variable_name] = next_offset;
    next_offset -= 4;
  }
};

struct SymbolTable {
  bool use_print = false;
  bool use_memory = false;

  std::map<std::string, ProcedureTable> table;
  std::string current_procedure = "";

  void add_procedure(const std::string &procedure_name, const Type return_type,
                     const std::vector<Type> &argument_types) {
    table[procedure_name].return_type = return_type;
    table[procedure_name].argument_types = argument_types;
  }

  void add_variable(const std::string &variable_name, const Type type) {
    table[current_procedure].add_variable(variable_name, type);
  }

  void use_variable(const std::string &variable_name) {
    table[current_procedure].used_variables.insert(variable_name);
  }

  bool contains_procedure(const std::string &procedure_name) const {
    return table.count(procedure_name) > 0;
  }

  Type get_return_type(const std::string &procedure_name) const {
    runtime_assert(contains_procedure(procedure_name),
                   "Undefined procedure " + procedure_name);
    return table.at(procedure_name).return_type;
  }

  std::vector<Type>
  get_argument_types(const std::string &procedure_name) const {
    runtime_assert(contains_procedure(procedure_name),
                   "Undefined procedure " + procedure_name);
    return table.at(procedure_name).argument_types;
  }

  void enter_procedure(const std::string &procedure) {
    current_procedure = procedure;
  }

  void leave_procedure() { current_procedure.clear(); }

  bool is_variable_used(const std::string &procedure_name,
                        const std::string &variable_name) const {
    runtime_assert(contains_procedure(procedure_name),
                   "Undefined procedure " + procedure_name);
    return table.at(procedure_name).used_variables.count(variable_name) > 0;
  }

  Type get_variable_type(const std::string &variable_name) const {
    const bool is_procedure = contains_procedure(variable_name);
    runtime_assert(contains_procedure(current_procedure),
                   "Undefined procedure " + current_procedure);
    const auto symbol_table = table.at(current_procedure).symbol_table;
    if (!is_procedure) {
      runtime_assert(symbol_table.count(variable_name) > 0,
                     "Undefined variable " + variable_name + " in procedure " +
                         current_procedure);
      return symbol_table.at(variable_name);
    } else {
      const auto it = symbol_table.find(variable_name);
      if (it == symbol_table.end())
        return Type::Void;
      return it->second;
    }
  }

  int get_variable_offset(const std::string &procedure_name,
                          const std::string &variable_name) const {

    runtime_assert(table.count(procedure_name) > 0,
                   "Undefined procedure " + procedure_name);
    const auto offset_table = table.at(procedure_name).offsets;
    runtime_assert(offset_table.count(variable_name) > 0,
                   "Undefined variable " + variable_name + " in procedure " +
                       procedure_name);
    const int num_params = table.at(procedure_name).argument_types.size();
    const int raw_offset = offset_table.at(variable_name);
    return raw_offset + 4 * num_params;
  }

  int get_number_of_local_variables(const std::string &procedure_name) const {
    runtime_assert(table.count(procedure_name) > 0,
                   "Undefined procedure " + procedure_name);
    const auto procedure_table = table.at(procedure_name);
    const int num_symbols = procedure_table.symbol_table.size();
    const int num_params = procedure_table.argument_types.size();
    return num_symbols - num_params;
  }

  void debug() {
    std::cout << "; -----------------------------------" << std::endl;
    std::cout << "; use_print: " << (use_print ? "true" : "false") << std::endl;
    std::cout << "; use_memory: " << (use_memory ? "true" : "false")
              << std::endl;
    const std::string old_procedure = current_procedure;
    for (const auto &[procedure, procedure_table] : table) {
      enter_procedure(procedure);
      std::cout << "; In procedure " << procedure << ": " << std::endl;

      const auto procedure_used_variables = procedure_table.used_variables;
      for (const auto &[var, type] : procedure_table.symbol_table) {
        const bool used = procedure_used_variables.count(var) > 0;
        std::cout << ";   " << var << ": " << type_to_string(type) << " @ "
                  << get_variable_offset(procedure, var) << " ("
                  << (used ? "used" : "not used") << ")" << std::endl;
      }
      leave_procedure();
    }
    enter_procedure(old_procedure);
    std::cout << "; -----------------------------------" << std::endl;
  }
};

std::vector<std::string> consume_stdin() {
  std::vector<std::string> result;
  std::string line;
  while (std::getline(std::cin, line))
    result.push_back(line);
  return result;
}

std::vector<std::string> split(const std::string &line) {
  std::vector<std::string> result;
  std::stringstream ss(line);
  std::string token;
  while (ss >> token) {
    result.push_back(token);
  }
  return result;
}

std::pair<size_t, std::shared_ptr<ParseNode>>
create_tree(const std::string &name, const std::vector<std::string> &lines,
            size_t idx = 0) {
  std::shared_ptr<ParseNode> result = std::make_shared<ParseNode>(name);
  if (name == ".EMPTY")
    return std::make_pair(idx, nullptr);

  assert(idx < lines.size());
  const std::string next_line = lines[idx++];
  result->production = next_line;
  const std::vector<std::string> tokens = split(next_line);
  assert(name == tokens[0]);

  if (islower(name[0])) {
    for (int i = 1; i < tokens.size(); ++i) {
      const std::string child_name = tokens[i];
      const auto &[new_idx, child] = create_tree(child_name, lines, idx);
      idx = new_idx;
      if (child != nullptr)
        result->children.push_back(child);
    }
  } else {
    assert(tokens.size() == 2);
    result->lexeme = tokens[1];
  }

  return std::make_pair(idx, result);
}

Type decl_to_type(std::shared_ptr<ParseNode> node) {
  runtime_assert(node->name == "dcl",
                 "Invalid decl_to_type, got " + node->name);
  return string_to_type(node->children[0]->production);
}

void populate_params(SymbolTable &table, const std::string &procedure_name,
                     std::shared_ptr<ParseNode> params) {
  if (params->production == "params") {
    table.add_procedure(procedure_name, Type::Int, {});
    return;
  }

  std::vector<Type> argument_types;
  std::shared_ptr<ParseNode> param_list = params->children[0];
  while (param_list->children.size() > 1) {
    assert(param_list->name == "paramlist");
    argument_types.push_back(decl_to_type(param_list->children[0]));
    param_list = param_list->children[2];
  }
  argument_types.push_back(decl_to_type(param_list->children[0]));
  table.add_procedure(procedure_name, Type::Int, argument_types);
}

std::vector<Type> arg_list_to_types(std::shared_ptr<ParseNode> arglist) {
  std::vector<Type> result;
  std::shared_ptr<ParseNode> cur = arglist;
  while (cur->children.size() > 1) {
    result.push_back(cur->children[0]->type);
    cur = cur->children[2];
  }
  result.push_back(cur->children[0]->type);
  return result;
}

void visit_all(SymbolTable &context, std::shared_ptr<ParseNode> &node,
               const std::function<void(SymbolTable &,
                                        std::shared_ptr<ParseNode>)> &post) {

  if (node->name == "procedure" || node->name == "main") {
    const std::string procedure_name =
        node->name == "procedure" ? node->children[1]->lexeme : "wain";
    context.enter_procedure(procedure_name);
    for (int i = 2; i < node->children.size(); ++i) {
      visit_all(context, node->children[i], post);
    }
    context.leave_procedure();
  } else {
    for (auto &child : node->children) {
      visit_all(context, child, post);
    }
  }
  post(context, node);
}

void populate_symbol_table(SymbolTable &table,
                           std::shared_ptr<ParseNode> node) {
  if (node->name == "dcl") {
    const std::string variable_name = node->children[1]->lexeme;
    const Type type = decl_to_type(node);
    table.add_variable(variable_name, type);
    return;
  }

  if (node->name == "procedure") {
    const std::string procedure_name = node->children[1]->lexeme;
    const auto params = node->children[3];
    populate_params(table, procedure_name, params);
  }

  if (node->name == "main") {
    const Type first_type = decl_to_type(node->children[3]);
    const Type second_type = decl_to_type(node->children[5]);
    table.add_procedure("wain", Type::Int,
                        std::vector<Type>({first_type, second_type}));
  }

  if (node->production == "factor ID") {
    const std::string variable_name = node->children[0]->lexeme;
    table.use_variable(variable_name);
  }

  if (node->name == "PRINTLN") {
    table.use_print = true;
  }

  if (node->name == "NEW" || node->name == "DELETE") {
    table.use_memory = true;
  }
}

void infer_and_check_types(SymbolTable &table,
                           std::shared_ptr<ParseNode> node) {
  const std::string node_name = node->name;
  const std::string node_production = node->production;
  if (node_name == "ID") {
    const std::string variable_name = node->lexeme;
    node->type = table.get_variable_type(variable_name);
    return;
  }

  // The type of a NUM is int.
  if (node_name == "NUM") {
    node->type = Type::Int;
    return;
  }

  // The type of a NULL token is int*.
  if (node_name == "NULL") {
    node->type = Type::IntStar;
    return;
  }

  // The type of a factor deriving NUM or NULL is the same as the type of that
  // token.
  if (node_production == "factor NUM" || node_production == "factor NULL") {
    node->type = node->children[0]->type;
    return;
  }

  // When factor derives ID, the derived ID must have a type, and the type of
  // the factor is the same as the type of the ID.
  if (node_production == "factor ID") {
    runtime_assert(node->children[0]->type != Type::Void,
                   "factor deriving ID did not have a type");
    node->type = node->children[0]->type;
    return;
  }

  // When lvalue derives ID, the derived ID must have a type, and the type of
  // the lvalue is the same as the type of the ID.
  if (node_production == "lvalue ID") {
    runtime_assert(node->children[0]->type != Type::Void,
                   "lvalue deriving ID did not have a type");
    node->type = node->children[0]->type;
    return;
  }

  // The type of a factor deriving LPAREN expr RPAREN is the same as the type of
  // the expr.
  if (node_production == "factor LPAREN expr RPAREN") {
    node->type = node->children[1]->type;
    return;
  }

  // The type of an lvalue deriving LPAREN lvalue RPAREN is the same as the type
  // of the derived lvalue.
  if (node_production == "lvalue LPAREN expr RPAREN") {
    node->type = node->children[1]->type;
    return;
  }

  // The type of a factor deriving AMP lvalue is int*. The type of the derived
  // lvalue (i.e. the one preceded by AMP) must be int.
  if (node_production == "factor AMP lvalue") {
    runtime_assert(node->children[1]->type == Type::Int,
                   "AMP was applied to non-int");
    node->type = Type::IntStar;
    return;
  }

  // The type of a factor or lvalue deriving STAR factor is int. The type of the
  // derived factor (i.e. the one preceded by STAR) must be int*.
  if (node_production == "factor STAR factor" ||
      node_production == "lvalue STAR factor") {
    runtime_assert(node->children[1]->type == Type::IntStar,
                   "STAR was applied to non-int-star");
    node->type = Type::Int;
    return;
  }

  // The type of a factor deriving NEW INT LBRACK expr RBRACK is int*. The type
  // of the derived expr must be int.
  if (node_production == "factor NEW INT LBRACK expr RBRACK") {
    runtime_assert(node->children[3]->type == Type::Int,
                   "Expression in new was not int");
    node->type = Type::IntStar;
    return;
  }

  // The type of a factor deriving ID LPAREN RPAREN is int. The procedure whose
  // name is ID must have an empty signature.
  if (node_production == "factor ID LPAREN RPAREN") {
    const std::string procedure_name = node->children[0]->name;
    runtime_assert(table.contains_procedure(procedure_name),
                   "Calling undefined procedure " + procedure_name +
                       " with no arguments");
    runtime_assert(table.get_return_type(procedure_name) == Type::Int,
                   "Procedure " + procedure_name + " did not return int");
    runtime_assert(table.get_argument_types(procedure_name).empty(),
                   "Procedure " + procedure_name +
                       " called with no arguments, but expected more");
    node->type = table.get_return_type(procedure_name);
    return;
  }

  // The type of a factor deriving ID LPAREN arglist RPAREN is int. The
  // procedure whose name is ID must have a signature whose length is equal to
  // the number of expr strings (separated by COMMA) that are derived from
  // arglist. Further the types of these expr strings must exactly match, in
  // order, the types in the procedure's signature.
  if (node_production == "factor ID LPAREN arglist RPAREN") {
    const std::string procedure_name = node->children[0]->lexeme;
    const auto parameter_types = table.get_argument_types(procedure_name);
    const auto argument_types = arg_list_to_types(node->children[2]);
    runtime_assert(argument_types == parameter_types,
                   "Invalid call to procedure " + procedure_name);
    node->type = Type::Int;
  }

  // The type of a term deriving factor is the same as the type of the derived
  // factor.
  if (node_production == "term factor") {
    node->type = node->children[0]->type;
    return;
  }

  // The type of a term directly deriving anything other than just factor is
  // int. The term and factor directly derived from such a term must have type
  // int.
  if (node_name == "term") {
    runtime_assert(node->children[0]->type == Type::Int,
                   "First argument in product expression was not int");
    runtime_assert(node->children[2]->type == Type::Int,
                   "Second argument in product expression was not int");
    node->type = Type::Int;
    return;
  }

  // The type of an expr deriving term is the same as the type of the derived
  // term.
  if (node_production == "expr term") {
    node->type = node->children[0]->type;
    return;
  }

  // When expr derives expr PLUS term:
  if (node_production == "expr expr PLUS term") {
    //     The derived expr and the derived term may both have type int, in
    //     which case the type of the expr deriving them is int.

    //     The derived expr may have type int* and the derived term may have
    //     type int, in which case the type of the expr deriving them is int*.

    //     The derived expr may have type int and the derived term may have type
    //     int*, in which case the type of the expr deriving them is int*.
    const std::map<std::pair<Type, Type>, Type> plus_types = {
        std::make_pair(std::make_pair(Type::Int, Type::Int), Type::Int),
        std::make_pair(std::make_pair(Type::IntStar, Type::Int), Type::IntStar),
        std::make_pair(std::make_pair(Type::Int, Type::IntStar), Type::IntStar),
    };

    const auto types =
        std::make_pair(node->children[0]->type, node->children[2]->type);
    runtime_assert(plus_types.count(types) > 0,
                   "Invalid types in PLUS expression");
    node->type = plus_types.at(types);
    return;
  }

  // When expr derives expr MINUS term:
  if (node_production == "expr expr MINUS term") {
    //     The derived expr and the derived term may both have type int, in
    //     which case the type of the expr deriving them is int.

    //     The derived expr may have type int* and the derived term may have
    //     type int, in which case the type of the expr deriving them is int*.

    //     The derived expr and the derived term may both have type int*, in
    //     which case the type of the expr deriving them is int.
    const std::map<std::pair<Type, Type>, Type> minus_types = {
        std::make_pair(std::make_pair(Type::Int, Type::Int), Type::Int),
        std::make_pair(std::make_pair(Type::IntStar, Type::Int), Type::IntStar),
        std::make_pair(std::make_pair(Type::IntStar, Type::IntStar), Type::Int),
    };

    const auto types =
        std::make_pair(node->children[0]->type, node->children[2]->type);
    runtime_assert(minus_types.count(types) > 0,
                   "Invalid types in MINUS expression");
    node->type = minus_types.at(types);
    return;
  }

  if (node_name == "main") {
    // The second dcl in the sequence directly derived from main must derive a
    // type that derives INT.
    const Type second_type = decl_to_type(node->children[5]);
    runtime_assert(second_type == Type::Int,
                   "Second argument to main was not int");

    // The expr in the sequence directly derived from main must have type int.
    const Type body_type = node->children[11]->type;
    runtime_assert(body_type == Type::Int, "Return type of main was not int");

    return;
  }

  // The expr in the sequence directly derived from procedure must have type
  // int.
  if (node_name == "procedure") {
    const Type body_type = node->children[9]->type;
    runtime_assert(body_type == Type::Int,
                   "Return type of procedure was not int");
    return;
  }

  // When statement derives lvalue BECOMES expr SEMI, the derived lvalue and the
  // derived expr must have the same type.
  if (node_production == "statement lvalue BECOMES expr SEMI") {
    runtime_assert(node->children[0]->type == node->children[2]->type,
                   "Assignment types differed");
    return;
  }

  // When statement derives PRINTLN LPAREN expr RPAREN SEMI, the derived expr
  // must have type int.
  if (node_production == "statement PRINTLN LPAREN expr RPAREN SEMI") {
    runtime_assert(node->children[2]->type == Type::Int,
                   "Printed expression was not int");
    return;
  }

  // When statement derives DELETE LBRACK RBRACK expr SEMI, the derived expr
  // must have type int*.
  if (node_production == "statement DELETE LBRACK RBRACK expr SEMI") {
    runtime_assert(node->children[3]->type == Type::IntStar,
                   "Deleted expression did not have type int*");
    return;
  }

  // Whenever test directly derives a sequence containing two exprs, they must
  // both have the same type.
  if (node_name == "test") {
    runtime_assert(node->children[0]->type == node->children[2]->type &&
                       node->children[0]->type != Type::Void,
                   "Comparison of two expressions with wrong types");
    node->type = Type::Int;
    return;
  }

  // When dcls derives dcls dcl BECOMES NUM SEMI, the derived dcl must derive a
  // sequence containing a type that derives INT.
  if (node_production == "dcls dcls dcl BECOMES NUM SEMI") {
    const Type type = decl_to_type(node->children[1]);
    runtime_assert(type == Type::Int,
                   "Declaration assigning to int was not declared as int");
    return;
  }

  // When dcls derives dcls dcl BECOMES NULL SEMI, the derived dcl must derive a
  // sequence containing a type that derives INT STAR.
  if (node_production == "dcls dcls dcl BECOMES NULL SEMI") {
    const Type type = decl_to_type(node->children[1]);
    runtime_assert(type == Type::IntStar,
                   "Declaration assigning to NULL was not declared as int*");
    return;
  }
}

// Code generation
struct Assembly {
  std::vector<Instruction> instructions;

  Assembly() = default;

  int num_instructions() const {
    int result = 0;
    for (const auto &instruction : instructions) {
      if (instruction.opcode <= Word)
        result++;
    }
    return result;
  }

  void annotate(const std::string &comment) {
    instructions.back().comment_value = comment;
  }

  void push(const int reg) {
    instructions.push_back(Instruction::sw(reg, -4, 30));
    instructions.push_back(Instruction::sub(30, 30, 4));
  }

  void pop(const int reg) {
    instructions.push_back(Instruction::add(30, 30, 4));
    instructions.push_back(Instruction::lw(reg, -4, 30));
  }

  void load_const(const int reg, const int value) {
    instructions.push_back(Instruction::lis(reg));
    instructions.push_back(Instruction::word(value));
  }
  void load_const(const int reg, const std::string &label) {
    instructions.push_back(Instruction::lis(reg));
    instructions.push_back(Instruction::word(label));
  }

  void save_registers() {
    push(1);
    push(2);
    push(6);
    push(7);
  }

  void pop_registers() {
    pop(7);
    pop(6);
    pop(2);
    pop(1);
  }

  void pop_and_discard(const int num_values) {
    assert(num_values >= 0);
    // Two options: one is to repeat 'add $30, $30, $4', stack_size times
    // The other is lis, word, add
    // The second option is better when stack_size > 3
    if (num_values > 3) {
      load_const(3, num_values * 4);
      instructions.push_back(Instruction::add(30, 30, 3));
    } else {
      for (int i = 0; i < num_values; ++i)
        instructions.push_back(Instruction::add(30, 30, 4));
    }
  }

  void generate_code(std::shared_ptr<ParseNode> node, SymbolTable &table);

  void print() const {
    for (const auto &instruction : instructions) {
      std::cout << instruction.to_string() << std::endl;
    }
  }
};

std::string generate_label(const std::string &label_type) {
  static std::map<std::string, int> next_indices;
  const int idx = next_indices[label_type];
  next_indices[label_type]++;
  return "_" + label_type + "_" + std::to_string(idx);
}

void Assembly::generate_code(std::shared_ptr<ParseNode> node,
                             SymbolTable &table) {
  const auto production = node->production;

  if (production == "start BOF procedures EOF") {
    if (table.use_print) {
      instructions.push_back(Instruction::import("print"));
    }
    if (table.use_memory) {
      instructions.push_back(Instruction::import("init"));
      instructions.push_back(Instruction::import("new"));
      instructions.push_back(Instruction::import("delete"));
    }
    instructions.push_back(Instruction::beq(0, 0, "wain"));
    generate_code(node->children[1], table);
  } else if (production == "procedures procedure procedures") {
    generate_code(node->children[0], table);
    generate_code(node->children[1], table);
  } else if (production == "procedures main") {
    generate_code(node->children[0], table);
  } else if (production == "procedure INT ID LPAREN params RPAREN LBRACE dcls "
                           "statements RETURN expr SEMI RBRACE") {
    const auto procedure_name = node->children[1]->lexeme;
    const auto decls = node->children[6];
    const auto statements = node->children[7];
    const auto expr = node->children[9];
    const int num_local_variables =
        table.get_number_of_local_variables(procedure_name);

    instructions.push_back(
        Instruction::comment("Code for procedure " + procedure_name));
    table.enter_procedure(procedure_name);
    instructions.push_back(Instruction::label(procedure_name));
    instructions.push_back(Instruction::sub(29, 30, 4));
    instructions.push_back(
        Instruction::comment("Decls for procedure " + procedure_name));
    generate_code(decls, table);
    instructions.push_back(
        Instruction::comment("Stmts for procedure " + procedure_name));
    generate_code(statements, table);
    instructions.push_back(
        Instruction::comment("Return value for procedure " + procedure_name));
    generate_code(expr, table);
    instructions.push_back(Instruction::comment(
        "Popping local variables for procedure " + procedure_name));
    pop_and_discard(num_local_variables);
    instructions.push_back(Instruction::jr(31));
    table.leave_procedure();
    instructions.push_back(
        Instruction::comment("Finished code for procedure " + procedure_name));

  } else if (production == "main INT WAIN LPAREN dcl COMMA dcl RPAREN LBRACE "
                           "dcls statements RETURN expr SEMI RBRACE") {
    const std::string first_arg_name = node->children[3]->children[1]->lexeme;
    const std::string second_arg_name = node->children[5]->children[1]->lexeme;
    const auto decls = node->children[8];
    const auto statements = node->children[9];
    const auto expr = node->children[11];

    table.enter_procedure("wain");
    instructions.push_back(Instruction::comment("Code for procedure wain"));
    instructions.push_back(Instruction::label("wain"));
    load_const(4, 4);
    annotate("Convention: $4 = 4");
    if (table.use_print) {
      load_const(10, "print");
      annotate("Convention: $10 = print");
    }
    load_const(11, 1);
    annotate("Convention: $11 = 1");

    push(1);
    push(2);

    // NOTE: We set $29 = $30 - 4 _AFTER_ pushing $1 and $2 to maintain
    // consistency with other procedure calls.
    instructions.push_back(Instruction::sub(29, 30, 4));
    instructions.push_back(Instruction::comment("Decls for procedure wain"));
    generate_code(decls, table);
    instructions.push_back(Instruction::comment("Stmts for procedure wain"));
    generate_code(statements, table);
    instructions.push_back(
        Instruction::comment("Return value for procedure wain"));
    generate_code(expr, table);
    instructions.push_back(Instruction::jr(31));
    table.leave_procedure();
    instructions.push_back(
        Instruction::comment("Finished code for procedure wain"));
  } else if (production == "params .EMPTY") {
  } else if (production == "params paramlist") {
    generate_code(node->children[0], table);
  } else if (production == "paramlist decl") {
    unreachable("params shouldn't generate code to place themselves onto the "
                "stack: the procedure and calls should do that");
  } else if (production == "paramlist dcl COMMA paramlist") {
    unreachable("params shouldn't generate code to place themselves onto the "
                "stack: the procedure and calls should do that");
  } else if (production == "type INT") {
    unreachable("types should never be expected to generate code");
  } else if (production == "type INT STAR") {
    unreachable("types should never be expected to generate code");
  } else if (production == "dcls .EMPTY") {
  } else if (production == "dcls dcls dcl BECOMES NUM SEMI") {
    generate_code(node->children[0], table);

    const std::string variable_name = node->children[1]->children[1]->lexeme;

    const int value = std::stoi(node->children[3]->lexeme);
    load_const(3, value);
    push(3);
    annotate("Pushing " + variable_name + " onto the stack");

  } else if (production == "dcls dcls dcl BECOMES NULL SEMI") {
    generate_code(node->children[0], table);

    const std::string variable_name = node->children[1]->children[1]->lexeme;
    push(0);
    annotate("Pushing " + variable_name + " onto the stack");
  } else if (production == "dcl type ID") {
    unreachable("dcl code generation should be handled in dcls");
  } else if (production == "statements .EMPTY") {
  } else if (production == "statements statements statement") {
    generate_code(node->children[0], table);
    generate_code(node->children[1], table);
  } else if (production == "statement lvalue BECOMES expr SEMI") {
    generate_code(node->children[0], table);          // $3 = addr of lvalue
    push(3);                                          // push addr onto stack
    generate_code(node->children[2], table);          // $3 = expr
    pop(5);                                           // $5 = addr
    instructions.push_back(Instruction::lw(3, 0, 5)); // MEM[$5 + 0] = $3
  } else if (production == "statement IF LPAREN test RPAREN LBRACE statements "
                           "RBRACE ELSE LBRACE statements RBRACE") {
    const auto test = node->children[2];
    const auto true_statements = node->children[5];
    const auto false_statements = node->children[9];
    const std::string else_label = generate_label("if_statement_else");
    const std::string endif_label = generate_label("if_statement_endif");

    generate_code(test, table);
    instructions.push_back(Instruction::beq(3, 0, else_label));
    generate_code(true_statements, table);
    instructions.push_back(Instruction::beq(0, 0, endif_label));
    instructions.push_back(Instruction::label(else_label));
    generate_code(false_statements, table);
    instructions.push_back(Instruction::label(endif_label));
  } else if (production ==
             "statement WHILE LPAREN test RPAREN LBRACE statements RBRACE") {
    const auto test = node->children[2];
    const auto statements = node->children[5];
    const auto loop_label = generate_label("while_loop");
    const auto end_while_label = generate_label("while_end_while");

    instructions.push_back(Instruction::label(loop_label));
    generate_code(test, table);
    instructions.push_back(Instruction::beq(3, 0, end_while_label));
    generate_code(statements, table);
    instructions.push_back(Instruction::beq(0, 0, loop_label));
    instructions.push_back(Instruction::label(end_while_label));
  } else if (production == "statement PRINTLN LPAREN expr RPAREN SEMI") {
    push(1);
    generate_code(node->children[2], table);
    instructions.push_back(Instruction::add(1, 3, 0));
    push(31);
    instructions.push_back(Instruction::jalr(10));
    pop(31);
    pop(1);
  } else if (production == "statement DELETE LBRACK RBRACK expr SEMI") {
    const auto expr = node->children[3];
    const auto skip_delete_label = generate_label("delete_skip_delete");
    generate_code(expr, table);
    instructions.push_back(Instruction::beq(3, 11, skip_delete_label));
    instructions.push_back(Instruction::add(1, 3, 0));
    push(31);
    load_const(5, "delete");
    instructions.push_back(Instruction::jalr(5));
    pop(31);
    instructions.push_back(Instruction::label(skip_delete_label));
  } else if (production == "test expr EQ expr") {
    generate_code(node->children[0], table);
    push(3);
    generate_code(node->children[2], table);
    pop(5);
    instructions.push_back(Instruction::slt(6, 3, 5));  // $6 = ($3 < $5)
    instructions.push_back(Instruction::slt(3, 5, 3));  // $3 = ($5 < $3)
    instructions.push_back(Instruction::add(3, 3, 6));  // $3 = ($3 != $5)
    instructions.push_back(Instruction::sub(3, 11, 3)); // $3 = 1 - $3
  } else if (production == "test expr NE expr") {
    generate_code(node->children[0], table);
    push(3);
    generate_code(node->children[2], table);
    pop(5);
    instructions.push_back(Instruction::slt(6, 3, 5)); // $6 = ($3 < $5)
    instructions.push_back(Instruction::slt(3, 5, 3)); // $3 = ($5 < $3)
    instructions.push_back(Instruction::add(3, 3, 6)); // $3 = ($3 != $5)
  } else if (production == "test expr LT expr") {
    generate_code(node->children[0], table);
    push(3);
    generate_code(node->children[2], table);
    pop(5);
    instructions.push_back(Instruction::slt(3, 5, 3));
  } else if (production == "test expr LE expr") {
    generate_code(node->children[0], table);
    push(3);
    generate_code(node->children[2], table);
    pop(5);
    instructions.push_back(Instruction::slt(3, 3, 5));
    instructions.push_back(Instruction::sub(3, 11, 3));
  } else if (production == "test expr GE expr") {
    generate_code(node->children[0], table);
    push(3);
    generate_code(node->children[2], table);
    pop(5);
    instructions.push_back(Instruction::slt(3, 5, 3));
    instructions.push_back(Instruction::sub(3, 11, 3));
  } else if (production == "test expr GT expr") {
    generate_code(node->children[0], table);
    push(3);
    generate_code(node->children[2], table);
    pop(5);
    instructions.push_back(Instruction::slt(3, 3, 5));
  } else if (production == "expr term") {
    generate_code(node->children[0], table);
  } else if (production == "expr expr PLUS term") {
    const auto lhs = node->children[0], rhs = node->children[2];
    if (lhs->type == Type::Int && rhs->type == Type::Int) {
      generate_code(lhs, table); // $3 = expr1
      push(3);                   // push expr1 onto stack
      generate_code(rhs, table); // $3 = expr2
      pop(5);                    // $5 = expr1
      instructions.push_back(Instruction::add(3, 5, 3)); // $3 = $5 + $3
    } else if (lhs->type == Type::IntStar && rhs->type == Type::Int) {
      generate_code(lhs, table);
      push(3);
      generate_code(rhs, table);
      instructions.push_back(Instruction::mult(3, 4));
      instructions.push_back(Instruction::mflo(3));
      pop(5);
      instructions.push_back(Instruction::add(3, 5, 3));
    } else if (lhs->type == Type::Int && rhs->type == Type::IntStar) {
      generate_code(lhs, table);
      instructions.push_back(Instruction::mult(3, 4));
      instructions.push_back(Instruction::mflo(3));
      push(3);
      generate_code(rhs, table);
      pop(5);
      instructions.push_back(Instruction::add(3, 5, 3));
    } else {
      unreachable("Invalid operand types for addition, typecheck should've "
                  "caught this");
    }
  } else if (production == "expr expr MINUS term") {
    const auto lhs = node->children[0], rhs = node->children[2];
    if (lhs->type == Type::Int && rhs->type == Type::Int) {
      generate_code(node->children[0], table); // $3 = expr1
      push(3);                                 // push expr1 onto stack
      generate_code(node->children[2], table); // $3 = expr2
      pop(5);                                  // $5 = expr1
      instructions.push_back(Instruction::sub(3, 5, 3)); // $3 = $5 - $3
    } else if (lhs->type == Type::IntStar && rhs->type == Type::Int) {
      generate_code(lhs, table);
      push(3);
      generate_code(rhs, table);
      instructions.push_back(Instruction::mult(3, 4));
      instructions.push_back(Instruction::mflo(3));
      pop(5);
      instructions.push_back(Instruction::sub(3, 5, 3));
    } else if (lhs->type == Type::IntStar && rhs->type == Type::IntStar) {
      generate_code(lhs, table);
      push(3);
      generate_code(rhs, table);
      pop(5);
      instructions.push_back(Instruction::sub(3, 5, 3));
      instructions.push_back(Instruction::div(3, 4));
      instructions.push_back(Instruction::mflo(3));
    } else {
      unreachable("Invalid operand types for subtraction, typecheck should've "
                  "caught this");
    }
  } else if (production == "term factor") {
    generate_code(node->children[0], table);
  } else if (production == "term term STAR factor") {
    generate_code(node->children[0], table);         // $3 = expr1
    push(3);                                         // push expr1 onto stack
    generate_code(node->children[2], table);         // $3 = expr2
    pop(5);                                          // $5 = expr1
    instructions.push_back(Instruction::mult(5, 3)); // hi:lo = $5 * $3
    instructions.push_back(Instruction::mflo(3));    // $3 = lo
  } else if (production == "term term SLASH factor") {
    generate_code(node->children[0], table);        // $3 = expr1
    push(3);                                        // push expr1 onto stack
    generate_code(node->children[2], table);        // $3 = expr2
    pop(5);                                         // $5 = expr1
    instructions.push_back(Instruction::div(5, 3)); // lo = $5 / $3
    instructions.push_back(Instruction::mflo(3));   // $3 = lo
  } else if (production == "term term PCT factor") {
    generate_code(node->children[0], table);        // $3 = expr1
    push(3);                                        // push expr1 onto stack
    generate_code(node->children[2], table);        // $3 = expr2
    pop(5);                                         // $5 = expr1
    instructions.push_back(Instruction::div(5, 3)); // hi = $5 % $3
    instructions.push_back(Instruction::mfhi(3));   // $3 = hi
  } else if (production == "factor ID") {
    const std::string variable_name = node->children[0]->lexeme;
    const int offset =
        table.get_variable_offset(table.current_procedure, variable_name);
    instructions.push_back(Instruction::lw(3, offset, 29));
    annotate("Grabbing ID: " + variable_name + " at offset " +
             std::to_string(offset));
  } else if (production == "factor NUM") {
    const auto constant_node = node->children[0];
    const int32_t value = std::stoi(constant_node->lexeme);
    load_const(3, value);
  } else if (production == "factor NULL") {
    // NOTE: We want dereferencing NULL to crash, so we set it to 1 instead of 0
    // so MIPS crashes on an unaligned memory access
    instructions.push_back(Instruction::add(3, 0, 11));
  } else if (production == "factor LPAREN expr RPAREN") {
    generate_code(node->children[1], table);
  } else if (production == "factor AMP lvalue") {
    // NOTE: The code generation for an lvalue puts its _address_ into $3
    generate_code(node->children[1], table);
  } else if (production == "factor STAR factor") {
    generate_code(node->children[1], table);
    instructions.push_back(Instruction::lw(3, 0, 3)); // $3 = MEM[$3 + 0]
  } else if (production == "factor NEW INT LBRACK expr RBRACK") {
    const auto expr = node->children[3];
    generate_code(expr, table);
    instructions.push_back(Instruction::add(1, 3, 0));
    push(31);
    load_const(5, "new");
    instructions.push_back(Instruction::jalr(5));
    pop(31);
    instructions.push_back(Instruction::bne(3, 0, 1));
    instructions.push_back(Instruction::add(3, 11, 0));
  } else if (production == "factor ID LPAREN RPAREN") {
    const auto procedure_name = node->children[0]->lexeme;
    push(29);
    push(31);
    save_registers();
    load_const(5, procedure_name);
    instructions.push_back(Instruction::jalr(5));
    pop_registers();
    pop(31);
    pop(29);
  } else if (production == "factor ID LPAREN arglist RPAREN") {
    const auto procedure_name = node->children[0]->lexeme;
    const int num_arguments = table.get_argument_types(procedure_name).size();

    push(29);
    push(31);
    save_registers();
    generate_code(node->children[2], table);
    load_const(5, procedure_name);
    instructions.push_back(Instruction::jalr(5));
    pop_and_discard(num_arguments);
    pop_registers();
    pop(31);
    pop(29);
  } else if (production == "arglist expr") {
    generate_code(node->children[0], table);
    push(3);
  } else if (production == "arglist expr COMMA arglist") {
    generate_code(node->children[0], table);
    push(3);
    generate_code(node->children[2], table);
  } else if (production == "lvalue ID") {
    const std::string variable_name = node->children[0]->lexeme;
    const int offset =
        table.get_variable_offset(table.current_procedure, variable_name);
    load_const(3, offset);                              // $3 = offset
    instructions.push_back(Instruction::add(3, 29, 3)); // $3 = $29 + $3
  } else if (production == "lvalue STAR factor") {
    // Because 'factor' must be a pointer, and lvalue's code generation expects
    // the address of the lvalue, we simply copy the factor's result into $3
    generate_code(node->children[1], table);
  } else if (production == "lvalue LPAREN lvalue RPAREN") {
    generate_code(node->children[1], table);
  } else {
    std::cerr << "WARN: Unknown production: " << production
              << ", could not generate code" << std::endl;
  }
}

int main() {
  try {
    const std::vector<std::string> lines = consume_stdin();
    auto tree = create_tree("start", lines).second;

    SymbolTable table;
    visit_all(table, tree, populate_symbol_table);
    visit_all(table, tree, infer_and_check_types);
    table.debug();

    // std::cout << tree->children[1]->wlp_code() << std::endl;
    // tree->print();

    Assembly program;
    program.generate_code(tree, table);
    program.print();
    std::cout << "; Program size: " << program.num_instructions()
              << " instructions" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
}
