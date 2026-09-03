#ifndef MUEHLE_MINI_MAX_ALPHA_BETA_RUN_ALPHA_BETA_VARS_H_
#define MUEHLE_MINI_MAX_ALPHA_BETA_RUN_ALPHA_BETA_VARS_H_

#include "muehle/mini_max/alpha_beta/common_thread_vars.h"
#include "muehle/mini_max/alpha_beta/knot_struct.h"
#include "muehle/mini_max/state_address_struct.h"

namespace muehle {
namespace mini_max {
namespace alpha_beta {

/* Thread specific variables for calculation */
struct RunAlphaBetaVars : public CommonThreadVars {
  Solver& r_solver;

  /* Array of size [(depth_of_full_tree - til_level) * max_num_branches] for
   * storage of the branches at each search depth */
  std::vector<KnotStruct> branch_array;

  /* Filled by game->GetSymmetricStates() */
  std::vector<StateAddressStruct> sym_states;

  /* Used to run the min-max calculation without database */
  KnotStruct* root_knot;

  RunAlphaBetaVars(RunAlphaBetaVars const& master);
  RunAlphaBetaVars(Solver& r_solver, unsigned int layer_number,
                   const std::wstring& filepath);
};

} /* namespace alpha_beta */
} /* namespace mini_max */
} /* namespace muehle */

#endif /* MUEHLE_MINI_MAX_ALPHA_BETA_RUN_ALPHA_BETA_VARS_H_ */