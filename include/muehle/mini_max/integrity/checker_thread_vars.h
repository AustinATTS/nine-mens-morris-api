#ifndef MUEHLE_MINI_MAX_INTEGRITY_CHECKER_THREAD_VARS_H_
#define MUEHLE_MINI_MAX_INTEGRITY_CHECKER_THREAD_VARS_H_

#include "muehle/mini_max/integrity/checker.h"
#include "muehle/mini_max/progress_counter.h"
#include "muehle/utils/thread_manager_class.h"

namespace muehle {
namespace mini_max {
namespace integrity {

/* Variables hold by each thread */
struct CheckerThreadVars : public ThreadManagerClass::ThreadVarsArrayItem {
  /* Padding to avoid cache coherence issues */
  char padding[64];

  /* Reference to the main checker object */
  Checker& r_checker;

  /* Layer number to be processed */
  unsigned int layer_number;

  /* For status output */
  ProgressCounter states_processed;

  std::vector<TwoBit> sub_value_in_database;
  std::vector<PlyInfoVarType> sub_ply_infos;
  std::vector<bool> has_cur_player_changed;
  std::vector<unsigned int> possibility_ids;

  /* Total number of states processed by all threads */
  int64_t& total_num_states_processed;

  CheckerThreadVars(CheckerThreadVars const& master);
  CheckerThreadVars(Checker& parent, unsigned int layer_number,
                    unsigned int max_num_branches,
                    int64_t rough_total_num_states_processed);

  void Reduce() override;
};

} /* namespace integrity */
} /* namespace mini_max */
} /* namespace muehle */

#endif /* MUEHLE_MINI_MAX_INTEGRITY_CHECKER_THREAD_VARS_H_ */