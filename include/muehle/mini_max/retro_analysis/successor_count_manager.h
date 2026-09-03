#ifndef MUEHLE_MINI_MAX_RETRO_ANALYSIS_SUCCESSOR_COUNT_MANAGER_H_
#define MUEHLE_MINI_MAX_RETRO_ANALYSIS_SUCCESSOR_COUNT_MANAGER_H_

#include "muehle/mini_max/database/database.h"
#include "muehle/mini_max/retro_analysis/state_queue.h"
#include "muehle/mini_max/retro_analysis/successor_count_array.h"
#include "muehle/mini_max/type_def.h"
#include "muehle/utils/logger.h"
#include "muehle/utils/thread_manager_class.h"

namespace muehle {
namespace mini_max {
namespace retro_analysis {

/* Class for calculating the number of succeding states for each state
 * - Thereby the database must already contain the knot values, for the invalid
 * states. all others must be win, drawn, or lost.
 * - The game interface must provide the functions 'SetSituation()' and
 * 'GetPredecessors()'
 * - Initialisation is done in parallel */
class SuccessorCountManager {
  friend class AddNumSuccedorsVars;

 public:
  SuccessorCountManager(Logger& log, ThreadManagerClass& tm,
                        database::Database& db, GameInterface& game,
                        std::vector<StateQueue>& states_to_process);
  ~SuccessorCountManager();
  bool Init(std::vector<unsigned int>& layers_to_calculate);
  bool IsReady();
  CountArrayVarType GetAndDecreaseCounter(unsigned int layer_number,
                                          StateNumberVarType state_number);

 protected:
  bool InitLayer(SuccessorCountArray& sca);
  bool CalcNumSuccedors(unsigned int layer_number);
  bool AddNumSuccedors(unsigned int layer_number);

  /* Logger, used for output */
  Logger& log;

  /* Thread manager, for parallel processing */
  ThreadManagerClass& tm;

  /* Database for storing calculated values */
  database::Database& db;

  /* Game inteface for getting the game specific information */
  GameInterface& game;

  /* Number of states processed by all threads */
  long long total_num_states_processed = 0;

  /* Number of states processed by all threads (rough estimate) */
  int64_t rough_total_num_states_processed = 0;

  /* Flag indicating if the layer has already been initialised */
  std::vector<bool> layer_processed;

  /* One successor count array for each layer in 'layers_to_calculate'. (Two
   * layers have to be considered at once). */
  std::vector<SuccessorCountArray*> succ_count_arrays;

  /* Queue of states to be processed, one for each thread */
  std::vector<StateQueue>& states_to_process;

  /* True if the count arrays are loaded from file, but the states_to_process
   * still needs to be filled */
  bool loaded_sca_from_file = false;

  /* Static thread functions */
  static DWORD AddNumSuccedorsThreadProc(void* p_parameter, int64_t index);
};

} /* namespace retro_analysis */
} /* namespace mini_max */
} /* namespace muehle */

#endif /* MUEHLE_MINI_MAX_RETRO_ANALYSIS_SUCCESSOR_COUNT_MANAGER_H_ */
