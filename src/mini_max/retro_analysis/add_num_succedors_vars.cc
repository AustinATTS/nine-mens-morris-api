#include "muehle/mini_max/retro_analysis/add_num_succedors_vars.h"

namespace muehle {

// Implementation goes here.

mini_max::retro_analysis::AddNumSuccedorsVars::AddNumSuccedorsVars(
    AddNumSuccedorsVars const& master) {
}

mini_max::retro_analysis::AddNumSuccedorsVars::AddNumSuccedorsVars(
    SuccessorCountManager& scm, unsigned int layer_number,
    int64_t& rough_total_num_states_processed) {
}

bool mini_max::retro_analysis::AddNumSuccedorsVars::StorePredecessorState(
    const StateAddressStruct& pred_state) {
}

void mini_max::retro_analysis::AddNumSuccedorsVars::Reduce() {
  ThreadVarsArrayItem::Reduce();
}

bool mini_max::retro_analysis::AddNumSuccedorsVars::Flush() {
}
}  // namespace muehle
