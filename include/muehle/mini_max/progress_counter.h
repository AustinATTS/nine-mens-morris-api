#ifndef MUEHLE_MINI_MAX_PROGRESS_COUNTER_H_
#define MUEHLE_MINI_MAX_PROGRESS_COUNTER_H_

#include "muehle/utils/logger.h"
#include "type_def.h"

namespace muehle {
namespace mini_max {

class ProgressCounter {
  int64_t states_processed_by_this_thread =
      0; /* Precise number of processed states by this thread */
  int64_t&
      rough_total_num_states_processed; /* Roughly the number of total processes
                                           states by all threads */

 public:
  ProgressCounter(int64_t& rough_total_num_states_processed);

  /* Called when a state is processed */
  void StateProcessed(Logger& log, StateNumberVarType num_knots_in_layer,
                      const std::wstring& text);
  int64_t GetStatesProcessedByThisThread() const;
};

}  // namespace mini_max
}  // namespace muehle

#endif  // MUEHLE_MINI_MAX_PROGRESS_COUNTER_H_
