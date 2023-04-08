
#include "ast_node.hpp"
#include "bril.hpp"
#include "bril_interpreter.hpp"
#include "dead_code_elimination.hpp"
#include "deduce_types.hpp"
#include "fold_constants.hpp"
#include "lexer.hpp"
#include "local_value_numbering.hpp"
#include "naive_mips_generator.hpp"
#include "parser.hpp"
#include "populate_symbol_table.hpp"
#include "simple_bril_generator.hpp"
#include "solve_generic_data_flow.hpp"
#include "symbol_table.hpp"
#include "util.hpp"

#include "parse_node.hpp"
#include <memory>

std::string read_file(const std::string &filename) {
  std::ifstream ifs(filename);
  if (!ifs.good()) {
    std::cerr << "Error: cannot open file " << filename << std::endl;
    exit(1);
  }
  std::stringstream buffer;
  buffer << ifs.rdbuf();
  return buffer.str();
}

std::string consume_stdin() {
  std::stringstream buffer;
  buffer << std::cin.rdbuf();
  return buffer.str();
}

std::shared_ptr<Program> program(const std::string &input) {
  const std::vector<Token> token_stream = Lexer(input).token_stream();
  const CFG cfg = load_cfg_from_file("references/productions.cfg");
  const EarleyTable table = EarleyParser(cfg).construct_table(token_stream);
  const std::shared_ptr<ParseNode> parse_tree = table.to_parse_tree();
  const std::shared_ptr<Program> program = construct_ast<Program>(parse_tree);
  return program;
}

std::shared_ptr<Program>
annotate_and_check_types(std::shared_ptr<Program> program) {
  // Populate symbol table
  PopulateSymbolTableVisitor symbol_table_visitor;
  program->accept_recursive(symbol_table_visitor);

  // Deduce types of intermediate expressions
  DeduceTypesVisitor deduce_types_visitor;
  program->accept_recursive(deduce_types_visitor);
  return program;
}

void debug_constant_folding() {
  const std::string input = "0 - (b - c)";
  const std::vector<Variable> variables = {
      Variable("a", Type::Int),
      Variable("b", Type::IntStar),
      Variable("c", Type::IntStar),
  };

  const std::vector<Token> token_stream = Lexer(input).token_stream();
  const CFG cfg = load_cfg_from_file("tests/arithmetic.cfg");
  const EarleyTable table = EarleyParser(cfg).construct_table(token_stream);
  const std::shared_ptr<ParseNode> parse_tree = table.to_parse_tree();
  const std::shared_ptr<Expr> expr = construct_ast<Expr>(parse_tree);

  SymbolTable mocked_table;
  mocked_table.add_procedure("MOCK");
  mocked_table.enter_procedure("MOCK");
  for (const Variable &variable : variables) {
    mocked_table.add_variable("MOCK", variable);
  }

  DeduceTypesVisitor deduce_types_visitor(mocked_table);
  expr->accept_recursive(deduce_types_visitor);

  const std::shared_ptr<Expr> simplified_expr = fold_constants(expr);

  simplified_expr->print(0);
}

void debug_dead_code_elimination() {
  using namespace bril;
  Function function("main", {}, bril::Type::Int);
  function.instructions = {
      bril::Instruction::constant("a", Literal(4, ::Type::Int)),
      bril::Instruction::constant("a", Literal(3, ::Type::Int)),
      bril::Instruction::constant("b", Literal(2, ::Type::Int)),
      bril::Instruction::constant("c", Literal(1, ::Type::Int)),
      bril::Instruction::add("d", "a", "b"),
      bril::Instruction::add("e", "c", "d"),
      bril::Instruction::add("e", "e", "e"),
      bril::Instruction::ret("d"),
  };
  ControlFlowGraph graph(function);
  std::cout << graph << std::endl;

  size_t num_iterations = 0, num_removed_lines = 0;
  while (true) {
    num_iterations++;
    const size_t old_num_removed_lines = num_removed_lines;
    num_removed_lines += graph.apply_local_pass(local_value_numbering);
    num_removed_lines += remove_global_unused_assignments(graph);
    num_removed_lines +=
        graph.apply_local_pass(remove_local_unused_assignments);
    if (num_removed_lines == old_num_removed_lines)
      break;
    std::cout << "After iteration " << num_iterations << std::endl;
    std::cout << graph << std::endl;
  }

  std::cout << "Converged after " << num_iterations << " iterations"
            << std::endl;
}

bril::Program get_bril(std::shared_ptr<Program> program) {
  bril::SimpleBRILGenerator generator;
  program->accept_simple(generator);
  return generator.program();
}

size_t apply_optimizations(bril::Program &program) {
  using namespace bril;
  size_t num_removed_lines = 0;
  while (true) {
    bool changed = false;
    const size_t old_num_removed_lines = num_removed_lines;
    num_removed_lines += program.apply_local_pass(local_value_numbering);
    num_removed_lines +=
        program.apply_global_pass(remove_global_unused_assignments);
    num_removed_lines +=
        program.apply_local_pass(remove_local_unused_assignments);
    if (num_removed_lines == old_num_removed_lines)
      break;
  }
  return num_removed_lines;
}

void compute_reaching_definitions(bril::Program &bril_program) {
  const std::string separator(80, '-');
  for (const auto &[name, cfg] : bril_program.cfgs) {
    const auto result = bril::ReachingDefinitions::solve(cfg);
    std::cout << "For procedure " << name << ": " << std::endl;
    for (size_t i = 0; i < cfg.blocks.size(); ++i) {
      std::cout << separator << std::endl;
      std::cout << "Block idx " << i << std::endl;
      const auto block_in = result.in[i];
      const auto block_out = result.out[i];
      const auto block = cfg.blocks[i];

      std::cout << "  Reaching definitions in: " << std::endl;
      for (const auto &[block_idx, instruction_idx] : block_in) {
        const auto instruction =
            cfg.blocks[block_idx].instructions[instruction_idx];
        std::cout << "    " << instruction << std::endl;
      }

      std::cout << std::endl;
      for (const auto &instruction : block.instructions) {
        std::cout << instruction << std::endl;
      }
      std::cout << std::endl;

      std::cout << "  Reaching definitions out: " << std::endl;
      for (const auto &[block_idx, instruction_idx] : block_out) {
        const auto instruction =
            cfg.blocks[block_idx].instructions[instruction_idx];
        std::cout << "    " << instruction << std::endl;
      }
    }
  }
}

int main(int argc, char **argv) {
  using namespace bril::interpreter;

  // debug_dead_code_elimination();
  // return 0;

  try {
    const std::string input = argc > 1 ? read_file(argv[1]) : consume_stdin();
    const auto program0 = program(input);
    const auto program = annotate_and_check_types(program0);

    // program->emit_c(std::cerr, 0); // before constant folding
    // const auto symbol_table = program->table;
    // std::cerr << symbol_table << std::endl;

    // Fold constants
    FoldConstantsVisitor fold_constants_visitor;
    program->accept_recursive(fold_constants_visitor);

    // program->print();
    program->emit_c(std::cerr, 0); // after constant folding

    bril::Program bril_program = get_bril(program);
    std::cout << bril_program << std::endl;

    for (auto &[name, cfg] : bril_program.cfgs) {
      if (!cfg.uses_pointers())
        cfg.convert_to_ssa();
    }

    const size_t num_removed_lines = apply_optimizations(bril_program);
    std::cout << bril_program << std::endl;
    std::cout << "Optimizations removed " << num_removed_lines << " lines"
              << std::endl;

    // std::cout << bril_program << std::endl;

    BRILInterpreter interpreter(bril_program);
    interpreter.run(std::cerr);

    // std::cout << bril_program << std::endl;

  } catch (const std::exception &e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
}
