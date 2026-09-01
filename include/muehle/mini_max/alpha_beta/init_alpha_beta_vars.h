#ifndef MUEHLE_MINI_MAX_ALPHA_BETA_INIT_ALPHA_BETA_VARS_H_
#define MUEHLE_MINI_MAX_ALPHA_BETA_INIT_ALPHA_BETA_VARS_H_

#include "muehle/mini_max/alpha_beta/common_thread_vars.h"

namespace muehle {
namespace mini_max {
namespace alpha_beta {

/* Thread specific variables for initialisation */
struct InitAlphaBetaVars : public CommonThreadVars {
  Solver& r_solver;
  bool init_already_done = false; /* True, when the file is complete and
                                     contains all stats of a layer */

  InitAlphaBetaVars(InitAlphaBetaVars const& master);
  InitAlphaBetaVars(Solver& r_solver, unsigned int layer_number,
                    const std::wstring& filepath);
};
}  // namespace alpha_beta
}  // namespace mini_max
}  // namespace muehle

#endif  // MUEHLE_MINI_MAX_ALPHA_BETA_INIT_ALPHA_BETA_VARS_H_
