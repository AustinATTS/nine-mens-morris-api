#include "muehle/mini_max/integrity/checker_thread_vars.h"

namespace muehle {

// Implementation goes here.

mini_max::integrity::CheckerThreadVars::CheckerThreadVars(
    CheckerThreadVars const& master) {
}

mini_max::integrity::CheckerThreadVars::CheckerThreadVars(Checker& parent,
    unsigned int layer_number, unsigned int max_num_branches,
    int64_t rough_total_num_states_processed) {
}

void mini_max::integrity::CheckerThreadVars::Reduce() {
  ThreadVarsArrayItem::Reduce();
}
}  // namespace muehle
