#ifndef MUEHLE_MINI_MAX_RETRO_ANALYSIS_SOLVER_H_
#define MUEHLE_MINI_MAX_RETRO_ANALYSIS_SOLVER_H_

#include "muehle/mini_max/database/database.h"
#include "muehle/utils/logger.h"
#include "muehle/utils/thread_manager_class.h"
#include "muehle/mini_max/retro_analysis/state_queue.h"
#include "muehle/mini_max/retro_analysis/successor_count_manager.h"

namespace muehle {
namespace mini_max {
namespace retro_analysis {

/* Class for the retro analysis */
class Solver {
  friend struct InitRetroAnalysisVars;
  friend class SuccessorCountArray;

 public:
  Solver(Logger& log, ThreadManagerClass& tm, database::Database& db,
         GameInterface game);
  ~Solver();
  bool CalcKnotValuesByRetroAnalysis(
      std::vector<unsigned int>& layers_to_calculate);

 private:
  int64_t
      rough_total_num_states_processed; /* Rough estimate of the total number of
                                           states to be processed */
  int64_t total_num_states_processed; /* Total number of states processed by all
                                         threads */
  std::vector<bool> layer_initialized; /* Flag indicating if the layer has
                                          already been initialized */
  std::vector<unsigned int>
      layers_to_calculate; /* Layers which shall be calculated */
  std::vector<StateQueue>
      states_to_process;  /* States already calculated, used as basis for
                             preceding states. One queue per thread. logger used
                             for output database, for storing the calculated
                             values */
  Logger& log;            /* Logger used for output */
  database::Database& db; /* database, for storing the calculated values */
  GameInterface
      game; /* Game interface for getting the game specific information */
  ThreadManagerClass& tm;    /* Thread manager, for parallel processing */
  SuccessorCountManager scm; /* Successor count manager */

  bool InitRetroAnalysis();
  bool PrepareCountArrays();
  bool PerformRetroAnalysis();
  bool ProcessPredecessor(StateQueue& queue,
                          const StateAddressStruct& cur_state,
                          const PredVars& pred_vars_state);
  size_t EstimateTotalNumberOfKnots();

  /* Static thread functions */
  static DWORD InitRetroAnalysisThreadProc(void* p_parameter, int64_t index);
  static DWORD PerformRetroAnalysisThreadProc(void* p_parameter);
};

}  // namespace retro_analysis
}  // namespace mini_max
}  // namespace muehle

#endif  // MUEHLE_MINI_MAX_RETRO_ANALYSIS_SOLVER_H_
