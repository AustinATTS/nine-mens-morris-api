#ifndef MUEHLE_MINI_MAX_ALPHA_BETA_SOLVER_H_
#define MUEHLE_MINI_MAX_ALPHA_BETA_SOLVER_H_

#include "muehle/mini_max/alpha_beta/knot_struct.h"
#include "muehle/mini_max/alpha_beta/run_alpha_beta_vars.h"
#include "muehle/mini_max/database/database.h"
#include "muehle/mini_max/state_info.h"
#include "muehle/utils/logger.h"
#include "muehle/utils/thread_manager_class.h"

namespace muehle {
namespace mini_max {
namespace alpha_beta {

class Solver {
  friend struct CommonThreadVars;
  friend struct InitAlphaBetaVars;
  friend struct RunAlphaBetaVars;

 public:
  Solver(Logger& log, ThreadManagerClass& tm, database::Database& db,
         GameInterface& game);
  bool GetBestChoice(unsigned int& choice, StateInfo& info_about_choices);
  bool CalcKnotValueByAlphaBeta(std::vector<unsigned int>& layers_to_calculate);
  void SetSearchDepth(unsigned int max_alpha_beta_search_depth);

 private:
  /* Logger, used for output */
  Logger& log;

  /* Database, for storing the calculated values */
  database::Database& db;

  /* Game interface, for getting the game specific information */
  GameInterface& game;

  /* Thread manager, for parallel processing */
  ThreadManagerClass& tm;

  /* Number of states processed by all threads */
  int64_t total_num_states_processed = 0;

  /* Number of states processed by all threads (roughly) */
  int64_t rough_total_num_states_processed = 0;

  /* Maximum search depth, equivalent to the maximum number of plies */
  unsigned int depth_of_full_tree = 0;

  /* Maximum number of branches/moves */
  unsigned int max_num_branches = 0;

  /* True if the database is currently being calculated */
  bool calc_database = false;

  std::mutex mutex;

  bool Init(unsigned int layer_number);
  bool Run(unsigned int layer_number);
  bool LetTheTreeGrow(KnotStruct& knot, RunAlphaBetaVars& rab_vars,
                      unsigned int til_level, float alpha, float beta);
  bool TryDatabase(KnotStruct& knot, const RunAlphaBetaVars& rab_vars,
                   unsigned int til_level, unsigned int& layer_number,
                   unsigned int& state_number);
  bool TryPossibilities(KnotStruct& knot, RunAlphaBetaVars& rab_vars,
                        unsigned int til_level,
                        unsigned int& max_won_freq_values_sub_moves,
                        float& alpha, float& beta);
  bool SaveInDatabase(const KnotStruct& knot, RunAlphaBetaVars& rab_vars,
                      unsigned int layer_number, unsigned int state_number);

  /* Static thread functions */

  /* Used to initialise the database calculation */
  static DWORD InitThreadProc(void* p_parameter, int64_t index);

  /* Used to run the database calculation */
  static DWORD RunThreadProc(void* p_parameter, int64_t index);

  /* Used to run the min-max calculation without database */
  static DWORD MinMaxThreadProc(void* p_parameter, int64_t index);
};

} /* namespace alpha_beta */
} /* namespace mini_max */
} /* namespace muehle */

#endif /* MUEHLE_MINI_MAX_ALPHA_BETA_SOLVER_H_ */