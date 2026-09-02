#ifndef MUEHLE_MINI_MAX_RETRO_ANALYSIS_ADD_NUM_SUCCEDORS_VARS_H_
#define MUEHLE_MINI_MAX_RETRO_ANALYSIS_ADD_NUM_SUCCEDORS_VARS_H_

#include "muehle/mini_max/pred_vars.h"
#include "muehle/mini_max/progress_counter.h"
#include "muehle/mini_max/state_address_struct.h"
#include "muehle/utils/thread_manager_class.h"
#include "muehle/mini_max/retro_analysis/successor_count_manager.h"

namespace muehle {
namespace mini_max {
namespace retro_analysis {

/* Thread specific variables for the function 'AddNumSuccedorsThreadProc()' */
class AddNumSuccedorsVars : public ThreadManagerClass::ThreadVarsArrayItem {
 public:
  friend class SuccessorCountManager;

  SuccessorCountManager& scm;    /* Pointer to all successor count arrays */
  unsigned int layer_number = 0; /* Current layer number being calculated */
  ProgressCounter
      states_processed; /* Number of states processed by the thread */
  std::vector<PredVars> pred_vars_array{
      MAX_NUM_PREDECESSORS}; /* Array containing the predecessor states */

  AddNumSuccedorsVars(AddNumSuccedorsVars const& master);
  AddNumSuccedorsVars(SuccessorCountManager& scm, unsigned int layer_number,
                      int64_t& rough_total_num_states_processed);
  bool StorePredecessorState(const StateAddressStruct& pred_state);
  void Reduce();

 private:
  const size_t pred_states_chunk_size =
      1000000; /* Chunk size for the buffer_pred_states */
  static std::mutex
      succ_count_array_mutex; /* Mutex for the successor count arrays */
  std::vector<StateAddressStruct>
      buffer_pred_states; /* Buffer for storing the predecessor states */
  std::vector<int>
      map_layer_number_to_sca_id; /* Map for storing the layer number to the id
                                     of the successor count array */
  char padding[64];               /* Padding to avoid cache coherence issues */

  bool Flush();
};

}  // namespace retro_analysis
}  // namespace mini_max
}  // namespace muehle

#endif  // MUEHLE_MINI_MAX_RETRO_ANALYSIS_ADD_NUM_SUCCEDORS_VARS_H_
