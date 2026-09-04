#include "muehle/mini_max/retro_analysis/init_retro_analysis_vars.h"

namespace muehle {

/* Copy constructor for InitRetroAnalysisVars, duplicates thread specific
 * variables from master. */
mini_max::retro_analysis::InitRetroAnalysisVars::InitRetroAnalysisVars(
    InitRetroAnalysisVars const& master)
    : retro_vars(master.retro_vars), CommonThreadVars(master) {}

/* Initialises thread specific variables for retro analysis, including
 * references to solver, layer number and file path. */
mini_max::retro_analysis::InitRetroAnalysisVars::InitRetroAnalysisVars(
    Solver& retro_vars, unsigned int layer_number, const std::wstring& filepath)
    : retro_vars(retro_vars),
      CommonThreadVars(
          layer_number, filepath,
          /* target_file_size: */ retro_vars.db.GetNumberOfKnots(layer_number),
          retro_vars.rough_total_num_states_processed,
          retro_vars.total_num_states_processed, retro_vars.log) {}

} /* namespace muehle */
