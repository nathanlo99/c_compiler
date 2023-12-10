
#include "ast_node.hpp"
#include "bril.hpp"
#include "bril_interpreter.hpp"
#include "bril_to_mips_generator.hpp"
#include "call_graph.hpp"
#include "call_graph_walk.hpp"
#include "canonicalize_conditions.hpp"
#include "canonicalize_names.hpp"
#include "data_flow/alias_analysis.hpp"
#include "data_flow/data_flow.hpp"
#include "data_flow/liveness_analysis.hpp"
#include "dead_code_elimination.hpp"
#include "deduce_types.hpp"
#include "global_value_numbering.hpp"
#include "lexer.hpp"
#include "local_value_numbering.hpp"
#include "mem_to_reg.hpp"
#include "naive_mips_generator.hpp"
#include "parser.hpp"
#include "populate_symbol_table.hpp"
#include "run_optimization.hpp"
#include "simple_bril_generator.hpp"
#include "symbol_table.hpp"
#include "timer.hpp"
#include "util.hpp"

#include "parse_node.hpp"
#include <memory>

std::string consume_stdin() {
  std::ostringstream buffer;
  buffer << std::cin.rdbuf();
  return buffer.str();
}

std::string read_file(const std::string &filename) {
  if (filename == "-")
    return consume_stdin();

  std::ifstream ifs(filename);
  debug_assert(ifs.good(), "Cannot open file {}", filename);
  std::ostringstream buffer;
  buffer << ifs.rdbuf();
  return buffer.str();
}

std::shared_ptr<Program> get_program(const std::string &input) {
  const std::vector<Token> token_stream = Lexer(input).token_stream();
  const ContextFreeGrammar grammar = load_default_grammar();
  const EarleyTable table = EarleyParser(grammar).construct_table(token_stream);
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

bril::Program get_optimized_bril_from_file(const std::string &filename) {
  auto bril_program = get_bril_from_file(filename);
  run_optimization_passes(bril_program);
  bril_program.convert_to_ssa();
  run_optimization_passes(bril_program);
  bril_program.convert_from_ssa();
  run_optimization_passes(bril_program);
  while (bril::optimize_call_graph(bril_program))
    ;
  bril_program.for_each_function(bril::canonicalize_names);
  return bril_program;
}

void lex(const std::string &filename) {
  const std::string input = read_file(filename);
  const std::vector<Token> token_stream = Lexer(input).token_stream();
  for (const auto &token : token_stream) {
    std::cout << token << std::endl;
  }
}

void build_ast(const std::string &filename) {
  const std::string input = read_file(filename);
  const std::shared_ptr<Program> program = get_program(input);
  program->print();
}

void emit_c(const std::string &filename) {
  const std::string input = read_file(filename);
  const auto program = get_program(input);
  program->emit_c(std::cout, 0);
}

void emit_naive_mips(const std::string &filename) {
  const std::string input = read_file(filename);
  const auto program = get_program(input);

  NaiveMIPSGenerator generator;
  program->accept_simple(generator);
  generator.print(std::cout);
}

void emit_bril(const std::string &filename) {
  const auto program = get_bril_from_file(filename);
  program.print_flattened(std::cout);
}

void to_ssa(const std::string &filename) {
  auto bril_program = get_bril_from_file(filename);
  std::cout << "Before optimizations: " << std::endl;
  std::cout << bril_program << std::endl;

  run_optimization_passes(bril_program);
  bril_program.convert_to_ssa();
  run_optimization_passes(bril_program);
  bril_program.for_each_function(bril::canonicalize_names);

  std::cout << bril_program << std::endl;

  bril_program.print_flattened(std::cout);
}

void ssa_round_trip(const std::string &filename) {
  auto bril_program = get_bril_from_file(filename);
  run_optimization_passes(bril_program);
  bril_program.convert_to_ssa();
  run_optimization_passes(bril_program);
  bril_program.convert_from_ssa();
  run_optimization_passes(bril_program);
  bril_program.print_flattened(std::cout);
}

// Interpret the program without any optimizations
void bare_interpret(const std::string &filename) {
  // Calls the BRIL interpreter on the given file.
  using namespace bril::interpreter;
  auto bril_program = get_bril_from_file(filename);
  bril_program.print_flattened(std::cerr);
  BRILInterpreter interpreter(std::cin, std::cout);
  interpreter.run(bril_program);
}

void interpret(const std::string &filename) {
  // Calls the BRIL interpreter on the given file.
  using namespace bril::interpreter;
  const auto bril_program = get_optimized_bril_from_file(filename);

  BRILInterpreter interpreter(std::cin, std::cout);
  bril_program.print_flattened(std::cerr);
  interpreter.run(bril_program);
}

void round_trip_interpret(const std::string &filename) {
  // Calls the BRIL interpreter on the given file.
  using namespace bril::interpreter;
  const std::string input = read_file(filename);
  const auto program = get_program(input);
  auto bril_program = get_bril(program);

  run_optimization_passes(bril_program);
  bril_program.convert_to_ssa();
  run_optimization_passes(bril_program);
  bril_program.convert_from_ssa();
  run_optimization_passes(bril_program);

  BRILInterpreter interpreter(std::cin, std::cout);
  bril_program.print_flattened(std::cerr);
  interpreter.run(bril_program);
}

void compute_dominators(const std::string &filename) {
  using util::operator<<;
  const auto program = get_bril_from_file(filename);
  program.for_each_function([&](const bril::ControlFlowGraph &function) {
    std::cout << "Function: " << function.name << std::endl;
    for (const auto &label : function.block_labels) {
      std::cout << "  Block: " << label << std::endl;
      std::cout << "  - Immediate dominator: "
                << function.immediate_dominator(label) << std::endl;
      std::cout << "  - Dominance frontier: "
                << function.dominance_frontier(label) << std::endl;
    }
  });
}

void compute_liveness(const std::string &filename) {
  using namespace bril;
  using util::operator<<;

  static const std::string separator(100, '-'), padding(50, ' ');

  auto program = get_optimized_bril_from_file(filename);
  program.for_each_function([](auto &function) {
    LivenessAnalysis analysis(function);
    const auto result = analysis.run();
    for (const auto &label : function.block_labels) {
      const auto &block = function.get_block(label);
      std::cout << separator << std::endl;
      std::cout << label << std::endl;

      for (size_t i = 0; i < block.instructions.size(); ++i) {
        const auto &live_in = result.get_data_in(label, i);
        const std::set<std::string> sorted_live_in(live_in.begin(),
                                                   live_in.end());
        std::cout << padding << "live variables: " << sorted_live_in
                  << std::endl;
        std::cout << block.instructions[i] << std::endl;
      }
      const auto &live_out = result.get_block_out(label);
      const std::set<std::string> sorted_live_out(live_out.begin(),
                                                  live_out.end());
      std::cout << padding << "live variables: " << sorted_live_out
                << std::endl;
    }
    std::cout << separator << std::endl;
  });
}

void compute_rig(const std::string &filename) {
  const auto program = get_optimized_bril_from_file(filename);
  const std::string separator(100, '-'), padding(50, ' ');

  program.for_each_function([=](const auto &function) {
    std::cout << separator << std::endl;
    std::cout << "Function: " << function.name << std::endl;
    std::cout << "Register interference graph: " << std::endl;
    std::cout << bril::RegisterInterferenceGraph(function);
  });
  std::cout << separator << std::endl;
}

void allocate_registers(const std::string &filename) {
  const auto program = get_optimized_bril_from_file(filename);
  const std::string separator(100, '-'), padding(50, ' ');

  const std::vector<Reg> available_registers = {
      Reg::R3,  Reg::R5,  Reg::R6,  Reg::R7,  Reg::R8,  Reg::R9,
      Reg::R10, Reg::R12, Reg::R13, Reg::R14, Reg::R15, Reg::R16,
      Reg::R17, Reg::R18, Reg::R19, Reg::R20, Reg::R21, Reg::R22,
      Reg::R23, Reg::R24, Reg::R25, Reg::R26, Reg::R27, Reg::R28};

  program.for_each_function([=](const auto &function) {
    std::cout << separator << std::endl;
    std::cout << "Function: " << function.name << std::endl;
    std::cout << "Register interference graph: " << std::endl;
    const auto register_allocation =
        bril::allocate_registers(function, available_registers);
    std::cout << register_allocation << std::endl;
  });
  std::cout << separator << std::endl;
}

void compute_call_graph(const std::string &filename) {
  using util::operator<<;
  auto program = get_bril_from_file(filename);
  const auto call_graph = bril::CallGraph(program);
  std::cout << call_graph << std::endl;
}

void generate_mips(const std::string &filename) {
  auto program = get_optimized_bril_from_file(filename);
  bril::optimize_call_graph(program);
  bril::BRILToMIPSGenerator generator(program);
  generator.print(std::cout);
}

void benchmark(const std::string &filename) {
  const std::string &input = read_file(filename);
  // 1. Lex the input
  const auto lexing_timer = ScopedTimer("1. Lexing");
  const std::vector<Token> token_stream = Lexer(input).token_stream();
  lexing_timer.stop();

  // 2. Parse
  const auto parsing_timer = ScopedTimer("2. Parsing");
  const ContextFreeGrammar grammar = load_default_grammar();
  const EarleyParser parser(grammar);
  const EarleyTable table = parser.construct_table(token_stream);
  const std::shared_ptr<ParseNode> parse_tree = table.to_parse_tree();
  parsing_timer.stop();

  // 3. Convert to AST
  const auto ast_construction_timer = ScopedTimer("3. AST construction");
  std::shared_ptr<Program> program = construct_ast<Program>(parse_tree);
  PopulateSymbolTableVisitor symbol_table_visitor;
  program->accept_recursive(symbol_table_visitor);
  DeduceTypesVisitor deduce_types_visitor;
  program->accept_recursive(deduce_types_visitor);
  CanonicalizeConditions canonicalize_conditions;
  program->accept_recursive(canonicalize_conditions);
  ast_construction_timer.stop();

  // 4. Convert to BRIL
  const auto bril_generation_timer = ScopedTimer("4. BRIL generation");
  bril::SimpleBRILGenerator bril_generator;
  program->accept_simple(bril_generator);
  bril::Program bril_program = bril_generator.program();
  bril_generation_timer.stop();

  // 5. Pre-SSA optimization
  const auto pre_ssa_optimization_timer =
      ScopedTimer("5. Pre-SSA optimization");
  run_optimization_passes(bril_program);
  pre_ssa_optimization_timer.stop();

  // 6. Convert to SSA
  const auto ssa_conversion_timer = ScopedTimer("6. Conversion to SSA");
  bril_program.convert_to_ssa();
  ssa_conversion_timer.stop();

  // 7. Post-SSA optimization
  const auto post_ssa_optimization_timer =
      ScopedTimer("7. Post-SSA optimization");
  run_optimization_passes(bril_program);
  post_ssa_optimization_timer.stop();

  // 8. Convert from SSA
  const auto convert_from_ssa_timer = ScopedTimer("8. Conversion from SSA");
  bril_program.convert_from_ssa();
  run_optimization_passes(bril_program);
  convert_from_ssa_timer.stop();

  // 9. Generate MIPS
  const auto mips_generation_timer = ScopedTimer("9. MIPS generation");
  bril::BRILToMIPSGenerator bril_to_mips_generator(bril_program);
  mips_generation_timer.stop();

  bril_to_mips_generator.print(std::cout);
}

void test_augmented_cfg(const std::string &filename) {
  const std::string input = read_file(filename);
  const std::vector<Token> token_stream = Lexer(input).token_stream();
  const ContextFreeGrammar grammar =
      load_grammar_from_file("references/augmented.cfg");
  const EarleyTable table = EarleyParser(grammar).construct_table(token_stream);
  const std::shared_ptr<ParseNode> parse_tree = table.to_parse_tree();
  std::shared_ptr<Program> program = construct_ast<Program>(parse_tree);

  // Analysis passes
  CanonicalizeConditions canonicalize_conditions;
  program->accept_recursive(canonicalize_conditions);
  PopulateSymbolTableVisitor symbol_table_visitor;
  program->accept_recursive(symbol_table_visitor);
  DeduceTypesVisitor deduce_types_visitor;
  program->accept_recursive(deduce_types_visitor);

  program->emit_c(std::cerr, 0);

  bril::SimpleBRILGenerator generator;
  program->accept_simple(generator);
  auto bril_program = generator.program();

  run_optimization_passes(bril_program);
  bril_program.convert_to_ssa();
  run_optimization_passes(bril_program);
  bril_program.convert_from_ssa();
  run_optimization_passes(bril_program);

  bril::interpreter::BRILInterpreter interpreter(std::cin, std::cout);
  interpreter.run(bril_program);
}

void inline_functions(const std::string &filename) {
  using namespace bril;
  using util::operator<<;
  auto program = get_optimized_bril_from_file(filename);

  while (true) {
    bool changed = false;
    std::unordered_set<std::string> to_inline;
    for (const auto &[name, function] : program.functions) {
      if (function.num_instructions() <= 20 || function.num_labels() <= 5) {
        to_inline.insert(name);
      }
    }
    std::cerr << "to_inline = " << to_inline << std::endl;
    for (const auto &func : to_inline) {
      const bool this_changed = program.inline_function("wain", func);
      changed |= this_changed;
    }
    const size_t optimized = run_optimization_passes(program);
    if (!changed && optimized == 0)
      break;
  }

  program.for_each_function(bril::canonicalize_names);
  std::cerr << program << std::endl;

  bril::BRILToMIPSGenerator generator(program);
  generator.print(std::cout);
}

void compute_aliases(const std::string &filename) {
  using namespace bril;
  using util::operator<<;
  auto program = get_bril_from_file(filename);
  for (const auto &[name, function] : program.functions) {
    std::cerr << "Function: " << name << std::endl;
    const auto alias_results = MayAliasAnalysis(function).run();
    for (const auto &label : function.block_labels) {
      const auto &block = function.get_block(label);
      std::cerr << "  Block: " << label << std::endl;
      for (size_t i = 0; i < block.instructions.size(); ++i) {
        const auto &instruction = block.instructions[i];
        std::cerr << "    " << instruction << std::endl;
        if (instruction.destination != "") {
          const auto &locations =
              alias_results.get_data_out(label, i).at(instruction.destination);
          if (!locations.empty())
            std::cerr << "      -> " << locations << std::endl;
        }
      }
      std::cerr << std::string(100, '-') << std::endl;
    }
    std::cerr << std::endl;
  }
  std::cout << std::string(100, '-') << std::endl;

  // program.apply_global_pass(promote_memory_to_registers);
  // std::cout << program << std::endl;

  // run_optimization_passes(program);
  // std::cout << program << std::endl;
}

const static std::vector<
    std::pair<std::string, std::function<void(const std::string &filename)>>>
    options = {
        {"--default", generate_mips},
        {"--debug", inline_functions},
        {"--lex", lex},
        {"--build-ast", build_ast},
        {"--bril", emit_bril},
        {"--compute-dominators", compute_dominators},
        {"--bare-interpret", bare_interpret},
        {"--interpret", interpret},
        {"--round-trip-interpret", round_trip_interpret},
        {"--emit-c", emit_c},
        {"--ssa", to_ssa},
        {"--ssa-round-trip", ssa_round_trip},
        {"--liveness", compute_liveness},
        {"--compute-rig", compute_rig},
        {"--allocate-registers", allocate_registers},
        {"--compute-call-graph", compute_call_graph},
        {"--emit-naive-mips", emit_naive_mips},
        {"--emit-mips", generate_mips},
        {"--inline-functions", inline_functions},
        {"--benchmark", benchmark},

        // Experimental options
        {"--augmented-cfg", test_augmented_cfg},
        {"--compute-aliases", compute_aliases},
};
const static std::map<std::string, std::function<void(const std::string &)>>
    options_map(options.begin(), options.end());

int main(int argc, char *argv[]) {
  try {
    debug_assert(argc >= 2, "Expected a filename");
    const std::string argument = argc > 2 ? argv[2] : "--default",
                      filename = argv[1];

    if (options_map.count(argument) == 0) {
      fmt::print(stderr, "Unknown option: {}\n", argument);
      fmt::print(stderr, "Options are:\n");
      for (const auto &[option, _] : options) {
        fmt::print(stderr, "  {}\n", option);
      }
      return 1;
    }

    {
      const auto total_timer = ScopedTimer("Total");
      options_map.at(argument)(filename);
    }

    Timer::print(std::cerr, 5.0);
  } catch (const std::exception &e) {
    fmt::print(stderr, "ERROR: {}\n", e.what());
    return 1;
  }
}
