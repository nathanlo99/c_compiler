
#include "ast_node.hpp"
#include "bril.hpp"
#include "bril_interpreter.hpp"
#include "bril_to_mips_generator.hpp"
#include "canonicalize_conditions.hpp"
#include "data_flow.hpp"
#include "dead_code_elimination.hpp"
#include "deduce_types.hpp"
#include "fold_constants.hpp"
#include "global_value_numbering.hpp"
#include "lexer.hpp"
#include "live_analysis.hpp"
#include "local_value_numbering.hpp"
#include "naive_mips_generator.hpp"
#include "parser.hpp"
#include "populate_symbol_table.hpp"
#include "simple_bril_generator.hpp"
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

  // Canonicalize boolean expressions
  CanonicalizeConditions canonicalize_conditions;
  program->accept_recursive(canonicalize_conditions);

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
    num_removed_lines += program.apply_global_pass(global_value_numbering);
    num_removed_lines += program.apply_global_pass(remove_unused_blocks);
    num_removed_lines += program.apply_global_pass(combine_extended_blocks);
    num_removed_lines +=
        program.apply_global_pass(remove_global_unused_assignments);
    num_removed_lines +=
        program.apply_local_pass(remove_local_unused_assignments);
    num_removed_lines +=
        program.apply_local_pass(remove_trivial_phi_instructions);
    num_removed_lines += program.apply_pass(remove_unused_parameters);
    if (num_removed_lines == old_num_removed_lines)
      break;
  }
  return num_removed_lines;
}

bril::Program get_optimized_bril_from_file(const std::string &filename) {
  auto bril_program = get_bril_from_file(filename);
  apply_optimizations(bril_program);
  bril_program.convert_to_ssa();
  apply_optimizations(bril_program);
  bril_program.convert_from_ssa();
  apply_optimizations(bril_program);
  return bril_program;
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
  generator.print(std::cout);
}

void test_to_ssa(const std::string &filename) {
  auto bril_program = get_bril_from_file(filename);
  std::cout << "Before optimizations: " << std::endl;
  std::cout << bril_program << std::endl;

  apply_optimizations(bril_program);
  bril_program.convert_to_ssa();
  apply_optimizations(bril_program);

  std::cout << bril_program << std::endl;

  bril_program.print_flattened(std::cout);
}

void test_ssa_round_trip(const std::string &filename) {
  auto bril_program = get_bril_from_file(filename);
  apply_optimizations(bril_program);
  bril_program.convert_to_ssa();
  apply_optimizations(bril_program);
  bril_program.convert_from_ssa();
  apply_optimizations(bril_program);
  bril_program.print_flattened(std::cout);
}

// Interpret the program without any optimizations
void bare_interpret(const std::string &filename) {
  // Calls the BRIL interpreter on the given file.
  using namespace bril::interpreter;
  auto bril_program = get_bril_from_file(filename);
  BRILInterpreter interpreter(bril_program);
  bril_program.print_flattened(std::cerr);
  interpreter.run(std::cout);
}

void interpret(const std::string &filename) {
  // Calls the BRIL interpreter on the given file.
  using namespace bril::interpreter;
  const auto bril_program = get_optimized_bril_from_file(filename);

  BRILInterpreter interpreter(bril_program);
  bril_program.print_flattened(std::cerr);
  interpreter.run(std::cout);
}

void round_trip_interpret(const std::string &filename) {
  // Calls the BRIL interpreter on the given file.
  using namespace bril::interpreter;
  const std::string input = read_file(filename);
  const auto program = get_program(input);
  auto bril_program = get_bril(program);

  apply_optimizations(bril_program);
  bril_program.convert_to_ssa();
  apply_optimizations(bril_program);
  bril_program.convert_from_ssa();
  apply_optimizations(bril_program);

  BRILInterpreter interpreter(bril_program);
  bril_program.print_flattened(std::cerr);
  interpreter.run(std::cout);
}

void debug_liveness(const std::string &filename) {
  using namespace bril;
  using util::operator<<;

  static const std::string separator(100, '-'), padding(50, ' ');

  auto program = get_bril_from_file(filename);
  apply_optimizations(program);
  program.convert_to_ssa();
  apply_optimizations(program);
  program.convert_from_ssa();
  apply_optimizations(program);

  for (const auto &[name, cfg] : program.cfgs) {
    LivenessAnalysis analysis(cfg);
    const auto result = analysis.run();
    for (const auto &label : cfg.block_labels) {
      const auto &data = result.at(label);
      const auto &block = cfg.get_block(label);
      std::cout << separator << std::endl;
      std::cout << label << std::endl;

      for (size_t i = 0; i < block.instructions.size(); ++i) {
        std::cout << padding << "live variables: " << data.live_variables[i]
                  << std::endl;
        std::cout << block.instructions[i] << std::endl;
      }
      std::cout << padding << "live variables: "
                << data.live_variables[block.instructions.size()] << std::endl;
    }
    std::cout << separator << std::endl;
  }
}

void compute_rig(const std::string &filename) {
  const auto program = get_optimized_bril_from_file(filename);
  const std::string separator(100, '-'), padding(50, ' ');

  for (const auto &[name, cfg] : program.cfgs) {
    std::cout << separator << std::endl;
    std::cout << "Function: " << name << std::endl;
    std::cout << "Register interference graph: " << std::endl;
    std::cout << bril::RegisterInterferenceGraph(cfg);
  }
  std::cout << separator << std::endl;
}

void allocate_registers(const std::string &filename) {
  const auto program = get_optimized_bril_from_file(filename);
  const std::string separator(100, '-'), padding(50, ' ');

  const std::vector<size_t> available_registers = {
      3,  5,  6,  7,  8,  9,  10, 12, 13, 14, 15, 16,
      17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28};

  for (const auto &[name, cfg] : program.cfgs) {
    std::cout << separator << std::endl;
    std::cout << "Function: " << name << std::endl;
    std::cout << "Register interference graph: " << std::endl;
    const auto register_allocation =
        bril::allocate_registers(cfg, available_registers);
    std::cout << register_allocation << std::endl;
  }
  std::cout << separator << std::endl;
}

void generate_mips(const std::string &filename) {
  const auto program = get_optimized_bril_from_file(filename);
  bril::BRILToMIPSGenerator generator(program);
  generator.print(std::cout);
}

void debug(const std::string &filename) {
  using namespace bril;
  auto program = get_optimized_bril_from_file(filename);

  std::set<std::string> to_inline = {"p", "eat", "q", "loong", "r"};
  auto &wain = program.wain();
  while (true) {
    bool changed = false;
    for (const auto &label : wain.block_labels) {
      auto &block = wain.get_block(label);
      for (size_t i = 0; i < block.instructions.size(); ++i) {
        const auto &instruction = block.instructions[i];
        if (instruction.opcode != bril::Opcode::Call ||
            to_inline.count(instruction.funcs[0]) == 0)
          continue;
        program.inline_function_call(wain.name, label, i);
        apply_optimizations(program);
        changed = true;
        break;
      }
      if (changed)
        break;
    }
    if (!changed)
      break;
  }
  program.convert_to_ssa();
  apply_optimizations(program);
  program.convert_from_ssa();
  apply_optimizations(program);

  std::cout << program << std::endl;
}

int main(int argc, char **argv) {
  try {
    runtime_assert(argc == 3, "Expected a filename and an option");
    const std::string argument = argv[2], filename = argv[1];

    const std::map<std::string, std::function<void(const std::string &)>>
        options = {
            {"--debug", debug},
            {"--lex", test_lexer},
            {"--parse", test_parser},
            {"--build-ast", test_build_ast},
            {"--bare-interpret", bare_interpret},
            {"--interpret", interpret},
            {"--round-trip-interpret", round_trip_interpret},
            {"--emit-c", test_emit_c},
            {"--ssa", test_to_ssa},
            {"--ssa-round-trip", test_ssa_round_trip},
            {"--liveness", debug_liveness},
            {"--compute-rig", compute_rig},
            {"--allocate-registers", allocate_registers},
            {"--emit-naive-mips", test_emit_mips},
            {"--emit-mips", generate_mips},
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
