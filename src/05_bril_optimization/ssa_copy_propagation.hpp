#pragma once

#include "bril.hpp"

namespace bril {

// Substitutes every use of `x` with `y` wherever `x = id y` -- always safe
// in SSA form, since y's definition dominates x's (a hard SSA validity
// requirement), and x's definition dominates every use of x, so y
// dominates everywhere x could be used too. A no-op outside SSA form.
// Doesn't itself delete the now-dead `id` instructions -- the existing DCE
// passes already do that; this just lets them see it a round sooner
// instead of waiting on GVN's own (correct, but incidental) resolution.
size_t ssa_copy_propagation(ControlFlowGraph &function);

} // namespace bril
