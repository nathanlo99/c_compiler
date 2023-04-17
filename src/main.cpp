
#include "ast_node.hpp"
#include "bril.hpp"
#include "bril_interpreter.hpp"
#include "dead_code_elimination.hpp"
#include "deduce_types.hpp"
#include "fold_constants.hpp"
#include "global_value_numbering.hpp"
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
  const CFG cfg = load_default_cfg();
  const EarleyTable table = EarleyParser(cfg).construct_table(token_stream);
  const std::shared_ptr<ParseNode> parse_tree = table.to_parse_tree();
  std::shared_ptr<Program> program = construct_ast<Program>(parse_tree);

  // Populate symbol table
  PopulateSymbolTableVisitor symbol_table_visitor;
  program->accept_recursive(symbol_table_visitor);

  // Deduce types of intermediate expressions
  DeduceTypesVisitor deduce_types_visitor;
  program->accept_recursive(deduce_types_visitor);

  return program;
}

bril::Program get_bril(const std::shared_ptr<Program> &program) {
  bril::SimpleBRILGenerator generator;
  program->accept_simple(generator);
  return generator.program();
}

bril::Program get_bril_from_file(const std::string &filename) {
  const std::string input = read_file(filename);
  const std::shared_ptr<Program> program = get_program(input);
  return get_bril(program);
}

size_t apply_optimizations(bril::Program &program) {
  using namespace bril;
  size_t num_removed_lines = 0;
  while (true) {
    const size_t old_num_removed_lines = num_removed_lines;
    num_removed_lines += program.apply_local_pass(local_value_numbering);
    num_removed_lines += program.apply_global_pass(remove_unused_blocks);
    num_removed_lines +=
        program.apply_global_pass(remove_global_unused_assignments);
    num_removed_lines +=
        program.apply_local_pass(remove_local_unused_assignments);
    num_removed_lines += program.apply_global_pass(remove_trivial_blocks);
    num_removed_lines +=
        program.apply_local_pass(remove_trivial_phi_instructions);
    if (num_removed_lines == old_num_removed_lines)
      break;
  }
  std::cerr << "Optimizations removed " << num_removed_lines << " lines"
            << std::endl;
  return num_removed_lines;
}

void test_lexer(const std::string &filename) {
  const std::string input = read_file(filename);
  const std::vector<Token> token_stream = Lexer(input).token_stream();
  for (const auto &token : token_stream) {
    std::cout << token << std::endl;
  }
}

void test_parser(const std::string &filename) {
  const std::string input = read_file(filename);
  const std::vector<Token> token_stream = Lexer(input).token_stream();
  const CFG cfg = load_default_cfg();
  const EarleyTable table = EarleyParser(cfg).construct_table(token_stream);
  const std::shared_ptr<ParseNode> parse_tree = table.to_parse_tree();
  parse_tree->print_preorder();
}

void test_build_ast(const std::string &filename) {
  const std::string input = read_file(filename);
  const std::shared_ptr<Program> program = get_program(input);
  program->print();
}

void compute_reaching_definitions(const std::string &filename) {
  auto bril_program = get_bril_from_file(filename);
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
              cfg.get_block(block_label).instructions[instruction_idx];
          std::cout << "    " << instruction << std::endl;
        }
      }

      std::cout << std::endl;

      const auto &block = cfg.get_block(label);
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
              cfg.get_block(block_label).instructions[instruction_idx];
          std::cout << "    " << instruction << std::endl;
        }
      }
    }
  }
}

void test_emit_c(const std::string &filename) {
  const std::string input = read_file(filename);
  const auto program = get_program(input);
  program->emit_c(std::cout, 0);
}

void test_emit_mips(const std::string &filename) {
  const std::string input = read_file(filename);
  const auto program = get_program(input);

  FoldConstantsVisitor fold_constants;
  program->accept_recursive(fold_constants);

  NaiveMIPSGenerator generator;
  program->accept_simple(generator);
  generator.print();
}

void test_to_ssa(const std::string &filename) {
  auto bril_program = get_bril_from_file(filename);
  std::cout << "Before optimizations: " << std::endl;
  std::cout << bril_program << std::endl;

  apply_optimizations(bril_program);
  for (auto &[name, cfg] : bril_program.cfgs) {
    cfg.convert_to_ssa();
  }

  std::cout << bril_program << std::endl;

  bril_program.print_flattened(std::cout);
}

void interpret(const std::string &filename) {
  // Calls the BRIL interpreter on the given file.
  using namespace bril::interpreter;
  const std::string input = read_file(filename);
  const auto program = get_program(input);
  auto bril_program = get_bril(program);

  apply_optimizations(bril_program);
  for (auto &[name, cfg] : bril_program.cfgs) {
    cfg.convert_to_ssa();
  }
  apply_optimizations(bril_program);

  BRILInterpreter interpreter(bril_program);
  // program->emit_c(std::cerr, 0);
  bril_program.print_flattened(std::cerr);
  interpreter.run(std::cout);
}

void debug() {
  using namespace bril;
  using bril::Type;
  using bril::Variable;
  std::vector<Instruction> instructions = {
      Instruction::label(".L1"),
      Instruction::add("u0", "a0", "b0"),
      Instruction::add("v0", "c0", "d0"),
      Instruction::add("w0", "e0", "f0"),
      Instruction::eq("cond", "u0", "u0"),
      Instruction::br("cond", ".L2", ".L3"),

      Instruction::label(".L2"),
      Instruction::add("x0", "c0", "d0"),
      Instruction::add("y0", "c0", "d0"),
      Instruction::jmp(".L4"),

      Instruction::label(".L3"),
      Instruction::add("u1", "a0", "b0"),
      Instruction::add("x1", "e0", "f0"),
      Instruction::add("y1", "e0", "f0"),
      Instruction::jmp(".L4"),

      Instruction::label(".L4"),
      Instruction::phi("u2", bril::Type::Int, {"u0", "u1"}, {".L2", ".L3"}),
      Instruction::phi("x2", bril::Type::Int, {"x0", "x1"}, {".L2", ".L3"}),
      Instruction::phi("y2", bril::Type::Int, {"y0", "y1"}, {".L2", ".L3"}),
      Instruction::add("z0", "u2", "y2"),
      Instruction::add("u3", "a0", "b0"),
      Instruction::ret("z0"),
  };
  Function function("main",
                    {Variable("a0", Type::Int), Variable("b0", Type::Int),
                     Variable("c0", Type::Int), Variable("d0", Type::Int),
                     Variable("e0", Type::Int), Variable("f0", Type::Int)},
                    Type::Int);
  function.instructions = instructions;
  ControlFlowGraph graph(function);

  runtime_assert(graph.is_in_ssa_form(), "Not in SSA form");

  std::cout << graph << std::endl;

  GlobalValueNumberingPass(graph).run_pass();
  remove_global_unused_assignments(graph);
  remove_trivial_blocks(graph);
  remove_unused_blocks(graph);
  graph.apply_local_pass(remove_trivial_phi_instructions);

  std::cout << graph << std::endl;
}

int main(int argc, char **argv) {

  // debug();
  // return 0;

  try {
    runtime_assert(argc == 3, "Expected a filename and an option");
    const std::string argument = argv[2], filename = argv[1];

    const std::map<std::string, std::function<void(const std::string &)>>
        options = {
            {"--lex", test_lexer},
            {"--parse", test_parser},
            {"--build-ast", test_build_ast},
            {"--interpret", interpret},
            {"--reaching-definitions", compute_reaching_definitions},
            {"--emit-c", test_emit_c},
            {"--ssa", test_to_ssa},
            {"--emit-mips", test_emit_mips},
        };

    if (options.count(argument) == 0) {
      std::cerr << "Unknown option: " << argument << std::endl;
      std::cerr << "Options are: " << std::endl;
      for (const auto &[option, _] : options) {
        std::cerr << "  " << option << std::endl;
      }
      return 1;
    }
    options.at(argument)(filename);

  } catch (const std::exception &e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 1;
  }
}
