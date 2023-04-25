
#pragma once

#include "bril.hpp"
#include "util.hpp"

namespace bril {

size_t remove_global_unused_assignments(ControlFlowGraph &graph);
size_t remove_local_unused_assignments(ControlFlowGraph &graph, Block &block);
size_t remove_unused_blocks(ControlFlowGraph &graph);
size_t remove_unused_functions(Program &program);
size_t remove_trivial_phi_instructions(ControlFlowGraph &, Block &block);
size_t combine_extended_blocks(ControlFlowGraph &function);
size_t remove_unused_parameters(Program &program);

} // namespace bril
