#include "muehle/mini_max/alpha_beta/init_alpha_beta_vars.h"

namespace muehle {

mini_max::alpha_beta::InitAlphaBetaVars::InitAlphaBetaVars(
    InitAlphaBetaVars const& master)
    : r_solver(master.r_solver), CommonThreadVars(master) {}

mini_max::alpha_beta::InitAlphaBetaVars::InitAlphaBetaVars(
    Solver& r_solver, unsigned int layer_number, const std::wstring& filepath)
    : r_solver(r_solver),
      CommonThreadVars(layer_number, filepath,
                       r_solver.db.GetNumberOfKnots(layer_number),
                       r_solver.rough_total_num_states_processed,
                       r_solver.total_num_states_processed, r_solver.log) {}

} /* namespace muehle */
