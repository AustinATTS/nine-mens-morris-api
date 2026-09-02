#include "muehle/mini_max/progress_counter.h"

namespace muehle {
namespace mini_max {

/* Constructor */
ProgressCounter::ProgressCounter(int64_t& rough_total_num_states_processed)
    : rough_total_num_states_processed(rough_total_num_states_processed) {}

/* Increments the number of processed states by one and print the progress to
 * the log */
void ProgressCounter::StateProcessed(Logger& log,
                                     StateNumberVarType num_knots_in_layer,
                                     const std::wstring& text) {
  states_processed_by_this_thread++;

  if (num_knots_in_layer == 0) {
    return; /* Avoid division by 0 */
  }

  /* Print status */
  if (states_processed_by_this_thread % OUTPUT_EVERY_N_STATES == 0) {
    rough_total_num_states_processed += OUTPUT_EVERY_N_STATES;
    int percentage =
        (int)((rough_total_num_states_processed * 100) / num_knots_in_layer);
    if (percentage > 100) {
      percentage = 100;
    }
    std::wstringstream wss;
    wss << text << rough_total_num_states_processed << " of "
        << num_knots_in_layer << " states being " << percentage << " %";
    log.Log(Logger::LogLevel::info, wss.str().c_str());
  }
}

int64_t ProgressCounter::GetStatesProcessedByThisThread() const {
  return states_processed_by_this_thread;
}

}  // namespace mini_max
}  // namespace muehle
