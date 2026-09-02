#include "muehle/mini_max/alpha_beta/run_alpha_beta_vars.h"
#include "muehle/mini_max/alpha_beta/solver.h"

namespace muehle {

mini_max::alpha_beta::RunAlphaBetaVars::RunAlphaBetaVars(
    RunAlphaBetaVars const& master)
    : CommonThreadVars(master),
      sym_states(master.sym_states),
      r_solver(master.r_solver) {
  branch_array.resize(master.r_solver.max_num_branches *
                      master.r_solver.depth_of_full_tree);
}

mini_max::alpha_beta::RunAlphaBetaVars::RunAlphaBetaVars(
    Solver& r_solver, unsigned int layer_number, const std::wstring& filepath)
    : r_solver(r_solver),
      CommonThreadVars(layer_number, filepath,
                       r_solver.db.GetNumberOfKnots(layer_number),
                       r_solver.rough_total_num_states_processed,
                       r_solver.total_num_states_processed, r_solver.log) {
  branch_array.resize(r_solver.max_num_branches * r_solver.depth_of_full_tree);
}

}  // namespace muehle
