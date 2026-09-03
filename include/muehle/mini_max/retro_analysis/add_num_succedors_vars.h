#ifndef MUEHLE_MINI_MAX_RETRO_ANALYSIS_ADD_NUM_SUCCEDORS_VARS_H_
#define MUEHLE_MINI_MAX_RETRO_ANALYSIS_ADD_NUM_SUCCEDORS_VARS_H_

#include "muehle/mini_max/pred_vars.h"
#include "muehle/mini_max/progress_counter.h"
#include "muehle/mini_max/retro_analysis/successor_count_manager.h"
#include "muehle/mini_max/state_address_struct.h"
#include "muehle/utils/thread_manager_class.h"

namespace muehle {
namespace mini_max {
namespace retro_analysis {

/* Thread specific variables for the function 'AddNumSuccedorsThreadProc()' */
class AddNumSuccedorsVars : public ThreadManagerClass::ThreadVarsArrayItem {
 public:
  friend class SuccessorCountManager;

  /* Pointer to all successor count arrays */
  SuccessorCountManager& scm;

  /* Current layer number being calculated */
  unsigned int layer_number = 0;

  /* Number of states processed by the thread */
  ProgressCounter states_processed;

  /* Array containing the predecessor states */
  std::vector<PredVars> pred_vars_array{MAX_NUM_PREDECESSORS};

  AddNumSuccedorsVars(AddNumSuccedorsVars const& master);
  AddNumSuccedorsVars(SuccessorCountManager& scm, unsigned int layer_number,
                      int64_t& rough_total_num_states_processed);
  bool StorePredecessorState(const StateAddressStruct& pred_state);
  void Reduce() override;

 private:
  /* Chunk size for the buffer_pred_states */
  const size_t pred_states_chunk_size = 1000000;

  /* Mutex for the successor count arrays */
  static std::mutex succ_count_array_mutex;

  /* Buffer for storing the predecessor states */
  std::vector<StateAddressStruct> buffer_pred_states;

  /* Map for storing the layer number to the id of the successor count array */
  std::vector<int> map_layer_number_to_sca_id;

  /* Padding to avoid cache coherence issues */
  char padding[64];

  bool Flush();
};

} /* namespace retro_analysis */
} /* namespace mini_max */
} /* namespace muehle */

#endif /* MUEHLE_MINI_MAX_RETRO_ANALYSIS_ADD_NUM_SUCCEDORS_VARS_H_ */
