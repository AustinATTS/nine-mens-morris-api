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
  std::vector<KnotStruct>
      branch_array; /* Array of size [(depth_of_full_tree - til_level) *
                       max_num_branches] for storage of the branches at each
                       search depth */
  std::vector<StateAddressStruct>
      sym_states; /* Filled by game->GetSymmetricStates() */
  KnotStruct*
      root_knot; /* Used to run the min-max calculation without database */

  RunAlphaBetaVars(RunAlphaBetaVars const& master);
  RunAlphaBetaVars(Solver& r_solver, unsigned int layer_number,
                   const std::wstring& filepath);
};

}  // namespace alpha_beta
}  // namespace mini_max
}  // namespace muehle

#endif  // MUEHLE_MINI_MAX_ALPHA_BETA_RUN_ALPHA_BETA_VARS_H_
