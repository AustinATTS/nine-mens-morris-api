#ifndef MUEHLE_MINI_MAX_RETRO_ANALYSIS_INIT_RETRO_ANALYSIS_VARS_H_
#define MUEHLE_MINI_MAX_RETRO_ANALYSIS_INIT_RETRO_ANALYSIS_VARS_H_

#include "muehle/mini_max/retro_analysis/solver.h"
#include "muehle/utils/logger.h"
#include "muehle/utils/thread_manager_class.h"
#include "muehle/mini_max/alpha_beta/common_thread_vars.h"
#include "muehle/mini_max/database/database.h"
#include "muehle/mini_max/retro_analysis/state_queue.h"
#include "muehle/mini_max/retro_analysis/successor_count_array.h"
#include "muehle/mini_max/type_def.h"

namespace muehle {
namespace mini_max {
namespace retro_analysis {

/* Thread specific variables for the function 'InitRetroAnalysis()' */
class InitRetroAnalysisVars : public CommonThreadVars {
 public:
  Solver& retro_vars; /* Reference to the solver class */

  InitRetroAnalysisVars(InitRetroAnalysisVars const& master);
  InitRetroAnalysisVars(Solver& solver, unsigned int layer_number,
                        const std::wstring& filepath);
};

}  // namespace retro_analysis
}  // namespace mini_max
}  // namespace muehle

#endif  // MUEHLE_MINI_MAX_RETRO_ANALYSIS_INIT_RETRO_ANALYSIS_VARS_H_
