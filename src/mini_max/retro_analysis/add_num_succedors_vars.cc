#include "muehle/mini_max/retro_analysis/add_num_succedors_vars.h"

namespace muehle {

/* Use one Mutex for all threads to modify succ_count_array */
std::mutex
    mini_max::retro_analysis::AddNumSuccedorsVars::succ_count_array_mutex = {};

/* Called by the master thread once for each thread */
mini_max::retro_analysis::AddNumSuccedorsVars::AddNumSuccedorsVars(
    AddNumSuccedorsVars const& master)
    : scm(master.scm),
      layer_number(master.layer_number),
      states_processed(master.states_processed),
      map_layer_number_to_sca_id(master.map_layer_number_to_sca_id) {
  buffer_pred_states.clear();
  buffer_pred_states.reserve(pred_states_chunk_size);
}

/* Called by the master thread once. */
mini_max::retro_analysis::AddNumSuccedorsVars::AddNumSuccedorsVars(
    SuccessorCountManager& scm, unsigned int layer_number,
    int64_t& rough_total_num_states_processed)
    : scm(scm),
      layer_number(layer_number),
      states_processed(rough_total_num_states_processed) {
  /* Make mapping */
  map_layer_number_to_sca_id.resize(scm.db.GetNumLayers(), -1);
  for (unsigned int id = 0; id < scm.succ_count_arrays.size(); id++) {
    map_layer_number_to_sca_id[scm.succ_count_arrays[id]->GetLayerNumber()] =
        id;
  }
}

/* Called by the worker threads to store the predecessor state in a file */
bool mini_max::retro_analysis::AddNumSuccedorsVars::StorePredecessorState(
    const StateAddressStruct& pred_state) {
  /* Only care about layers for which the successor count array shall be
   * calculated */
  if (map_layer_number_to_sca_id[pred_state.layer_number] < 0) {
    return true;
  }

  /* Add this state to the buffer */
  buffer_pred_states.push_back(pred_state);

  /* Check if the buffer is full. If so, Flush() */
  if (buffer_pred_states.size() >= pred_states_chunk_size) {
    if (!Flush()) {
      return scm.log.Log(Logger::LogLevel::error,
                         L"AddNumSuccedorsVars::StorePredecessorState(): "
                         L"Flush() returned false;");
    }
  }
  return true;
}

/* Called by the master thread once for each thread */
void mini_max::retro_analysis::AddNumSuccedorsVars::Reduce() {
  if (!Flush()) {
    scm.log.Log(Logger::LogLevel::error,
                L"AddNumSuccedorsVars::Reduce(): Flush() returned false!");
    return;
  }
  scm.total_num_states_processed +=
      states_processed.GetStatesProcessedByThisThread();
}

bool mini_max::retro_analysis::AddNumSuccedorsVars::Flush() {
  /* Lock since IncreaseCounter() is not thread safe */
  std::lock_guard<std::mutex> lock(succ_count_array_mutex);

  for (auto& pred_state : buffer_pred_states) {
    if (pred_state.layer_number >= map_layer_number_to_sca_id.size()) {
      return scm.log.Log(
          Logger::LogLevel::error,
          L"AddNumSuccedorsVars::Flush(): Layer number is out of range!");
    }

    if (map_layer_number_to_sca_id[pred_state.layer_number] < 0) {
      return scm.log.Log(Logger::LogLevel::error,
                         L"AddNumSuccedorsVars::Flush(): Layer number is not "
                         L"in the list of layers to calculate!");
    }

    /* Add this state as possible move, for the preceeding state */
    if (scm.succ_count_arrays
            [map_layer_number_to_sca_id[pred_state.layer_number]]
                ->IncreaseCounter(pred_state.state_number) ==
        COUNT_ARRAY_MAX_VALUE) {
      return scm.log.Log(Logger::LogLevel::error,
                         L"AddNumSuccedorsVars::StorePredecessorState(): "
                         L"Counter is at maximum value!");
    }
  }
  buffer_pred_states.clear();

  return true;
}

}  /* namespace muehle */
