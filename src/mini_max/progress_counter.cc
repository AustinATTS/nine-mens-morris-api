#include "muehle/mini_max/progress_counter.h"

namespace muehle {

// Implementation goes here.

mini_max::ProgressCounter::ProgressCounter(
    int64_t& rough_total_num_states_processed) {
}

void mini_max::ProgressCounter::StateProcessed(Logger& log,
    StateNumberVarType num_knots_in_layer, const std::wstring& text) {
}

int64_t mini_max::ProgressCounter::GetStatesProcessedByThisThread() const {
}
}  // namespace muehle
