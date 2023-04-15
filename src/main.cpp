
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

std::shared_ptr<Program> get_program(const std::string &input) {
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
  std::cout << bril_program << std::endl;

  const std::string separator(80, '-');
  for (const auto &[name, cfg] : bril_program.cfgs) {
    const auto result = bril::ReachingDefinitions::solve(cfg);
    std::cout << "For procedure " << name << ": " << std::endl;
    for (const auto &label : cfg.block_labels) {
      std::cout << separator << std::endl;
      std::cout << "Block label " << label << std::endl;
      const auto block_in = result.in.at(label);
      const auto block_out = result.out.at(label);

      std::cout << "  Reaching definitions in: " << std::endl;
      for (const auto &[destination, block_label, instruction_idx] : block_in) {
        if (block_label == "__param") {
          std::cout << "    param: " << destination << std::endl;
        } else {
          const auto instruction =
              cfg.blocks.at(block_label).instructions[instruction_idx];
          std::cout << "    " << instruction << std::endl;
        }
      }

      std::cout << std::endl;

      const auto &block = cfg.blocks.at(label);
      for (const auto &instruction : block.instructions) {
        std::cout << instruction << std::endl;
      }
      std::cout << std::endl;

      std::cout << "  Reaching definitions out: " << std::endl;
      for (const auto &[destination, block_label, instruction_idx] :
           block_out) {
        if (block_label == "__param") {
          std::cout << "    param: " << destination << std::endl;
        } else {
          const auto instruction =
              cfg.blocks.at(block_label).instructions[instruction_idx];
          std::cout << "    " << instruction << std::endl;
        }
      }
    }
  }
}

void test_to_ssa(const std::string &filename) {
  using namespace bril::interpreter;
  const std::string input = read_file(filename);
  const auto program0 = get_program(input);
  const auto program = annotate_and_check_types(program0);
  auto bril_program = get_bril(program);
  std::cout << "Before optimizations: " << std::endl;
  std::cout << bril_program << std::endl;

  apply_optimizations(bril_program);
  for (auto &[name, cfg] : bril_program.cfgs) {
    cfg.convert_to_ssa();
  }
  bril_program.print_flattened();
}

void interpret(const std::string &filename) {
  // Calls the BRIL interpreter on the given file.
  using namespace bril::interpreter;
  const std::string input = read_file(filename);
  const auto program0 = get_program(input);
  const auto program = annotate_and_check_types(program0);
  auto bril_program = get_bril(program);
  apply_optimizations(bril_program);
  for (auto &[name, cfg] : bril_program.cfgs) {
    cfg.convert_to_ssa();
  }
  apply_optimizations(bril_program);

  BRILInterpreter interpreter(bril_program);
  program->emit_c(std::cerr, 0);
  bril_program.print_flattened();
  interpreter.run(std::cerr);
}

int main(int argc, char **argv) {
  using namespace bril::interpreter;

  try {
    runtime_assert(argc == 3, "Expected a filename and an option");
    const std::string argument = argv[2], filename = argv[1];

    if (argument == "--reaching-definitions") {
      const std::string input = read_file(filename);
      const auto program0 = get_program(input);
      const auto program = annotate_and_check_types(program0);
      auto bril_program = get_bril(program);
      compute_reaching_definitions(bril_program);
      return 0;
    }

    if (argument == "--emit-c") {
      const std::string input = read_file(filename);
      const auto program0 = get_program(input);
      const auto program = annotate_and_check_types(program0);
      program->emit_c(std::cerr, 0);
      return 0;
    }

    if (argument == "--ssa") {
      test_to_ssa(filename);
      return 0;
    }

    if (argument == "--emit-mips") {
      const std::string input = read_file(filename);
      const auto program0 = get_program(input);
      auto program = annotate_and_check_types(program0);

      FoldConstantsVisitor fold_constants;
      program->accept_recursive(fold_constants);

      NaiveMIPSGenerator generator;
      program->accept_simple(generator);
      generator.print();
      return 0;
    }

    if (argument == "--interp") {
      interpret(filename);
      return 0;
    }

    std::cerr << "Unknown option: " << argument << std::endl;

  } catch (const std::exception &e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
}
