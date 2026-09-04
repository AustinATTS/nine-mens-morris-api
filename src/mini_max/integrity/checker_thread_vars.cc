#include "muehle/mini_max/integrity/checker_thread_vars.h"

namespace muehle {

mini_max::integrity::CheckerThreadVars::CheckerThreadVars(
    CheckerThreadVars const& master)
    : r_checker(master.r_checker),
      states_processed(master.states_processed),
      layer_number(master.layer_number),
      total_num_states_processed(master.total_num_states_processed) {
  sub_value_in_database = master.sub_value_in_database;
  sub_ply_infos = master.sub_ply_infos;
  has_cur_player_changed = master.has_cur_player_changed;
}

mini_max::integrity::CheckerThreadVars::CheckerThreadVars(
    Checker& parent, unsigned int layer_number, unsigned int max_num_branches,
    int64_t rough_total_num_states_processed)
    : r_checker(parent),
      layer_number(layer_number),
      states_processed(rough_total_num_states_processed),
      total_num_states_processed(parent.num_states_processed) {
  sub_value_in_database.resize(max_num_branches);
  sub_ply_infos.resize(max_num_branches);
  has_cur_player_changed.resize(max_num_branches);
}

void mini_max::integrity::CheckerThreadVars::Reduce() {
  total_num_states_processed +=
      this->states_processed.GetStatesProcessedByThisThread();
}

} /* namespace muehle */
