

#include "bril.hpp"
#include "dead_code_elimination.hpp"
#include "global_value_numbering.hpp"
#include "local_value_numbering.hpp"

inline size_t run_optimization_passes(bril::Program &program) {
  using namespace bril;
  size_t num_removed_lines = 0;
  while (true) {
    const size_t old_num_removed_lines = num_removed_lines;
    num_removed_lines += program.apply_pass(remove_unused_functions);
    num_removed_lines +=
        program.apply_global_pass(remove_global_unused_assignments);
    num_removed_lines +=
        program.apply_local_pass(remove_local_unused_assignments);
    num_removed_lines += program.apply_local_pass(local_value_numbering);
    num_removed_lines += program.apply_global_pass(global_value_numbering);
    num_removed_lines +=
        program.apply_local_pass(remove_trivial_phi_instructions);
    num_removed_lines += program.apply_pass(remove_unused_parameters);
    num_removed_lines += program.apply_global_pass(combine_extended_blocks);
    num_removed_lines += program.apply_global_pass(remove_unused_blocks);
    if (num_removed_lines == old_num_removed_lines)
      break;
  }
  return num_removed_lines;
}
