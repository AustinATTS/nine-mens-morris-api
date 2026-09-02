#include "muehle/mini_max/retro_analysis/successor_count_manager.h"

#include "muehle/mini_max/retro_analysis/add_num_succedors_vars.h"
#include "muehle/mini_max/retro_analysis/successor_count_file_storage.h"
#include "muehle/mini_max/return_values.h"

namespace muehle {

/* Constructor for the successor count manager */
mini_max::retro_analysis::SuccessorCountManager::SuccessorCountManager(
    Logger& log, ThreadManagerClass& tm, database::Database& db,
    GameInterface& game, std::vector<StateQueue>& states_to_process)
    : log(log),
      tm(tm),
      db(db),
      game(game),
      states_to_process(states_to_process) {}

/* Delete successor count manager and all successor count arrays */
mini_max::retro_analysis::SuccessorCountManager::~SuccessorCountManager() {
  for (auto& sca : succ_count_arrays) {
    SAFE_DELETE(sca);
  }
}

/* Read from file or calculate the number of succeding staes for each state all
 * layers to calculate */
bool mini_max::retro_analysis::SuccessorCountManager::Init(
    std::vector<unsigned int>& layers_to_calculate) {
  /* Checks */
  for (auto layer_number : layers_to_calculate) {
    if (layer_number >= db.GetNumLayers()) {
      log << "ERROR: Layer number is out of range!\n";
      return ReturnValues::FalseOrStop();
    }
  }
  cur_action = Activity::prepare_count_array;

  /* Clear old data */
  for (auto& sca : succ_count_arrays) {
    SAFE_DELETE(sca);
  }
  succ_count_arrays.clear();
  layer_processed.clear();

  /* Allocate memory for the successor count arrays, one for each layer in
   * 'layers_to_calculate */
  for (size_t id = 0; id < layers_to_calculate.size(); id++) {
    succ_count_arrays.push_back(
        new SuccessorCountArray(log, db, layers_to_calculate[id]));
  }

  /* Prepare file read/write */
  std::vector<SuccessorCountFileStorage::LayerInfoStruct> layers_infos;
  layers_infos.reserve(layers_to_calculate.size());
  for (auto& sca : succ_count_arrays) {
    layers_infos.push_back(SuccessorCountFileStorage::LayerInfoStruct{
        sca->layer_number, db.GetNumberOfKnots(sca->layer_number), *sca});
  }
  SuccessorCountFileStorage scfs =
      SuccessorCountFileStorage(log, db.GetFileDirectory(), layers_infos);

  /* Try to load the count arrays from file */
  loaded_sca_from_file = scfs.Read();

  for (auto& sca : succ_count_arrays) {
    if (!InitLayer(*sca)) {
      log << "ERROR: Could not initialise count array for layer "
          << sca->GetLayerNumber() << "!\n";
      return ReturnValues::FalseOrStop();
    }
  }

  /* Save the count arrays to file */
  if (!loaded_sca_from_file) {
    if (!scfs.Write()) {
      log << "ERROR: Could not save count arrays to file!\n";
      return ReturnValues::FalseOrStop();
    }
  }

  return true;
}

/* Checks if all successor count arrays are initialised */
bool mini_max::retro_analysis::SuccessorCountManager::IsReady() {
  if (!succ_count_arrays.size()) {
    return false;
  }
  for (auto& sca : succ_count_arrays) {
    if (sca->GetLayerNumber() >= db.GetNumLayers()) {
      return false;
    }
  }
  return true;
}

/* Get the number of succeding states for the state and decrease the counter by
 * one in a thread safe way */
mini_max::CountArrayVarType
mini_max::retro_analysis::SuccessorCountManager::GetAndDecreaseCounter(
    unsigned int layer_number, StateNumberVarType state_number) {
  for (auto& sca : succ_count_arrays) {
    if (sca->GetLayerNumber() == layer_number) {
      return sca->DecreaseCounter(state_number);
    };
  }
  return COUNT_ARRAY_MAX_VALUE;
}

/* Initialise the count array for the current layer */
bool mini_max::retro_analysis::SuccessorCountManager::InitLayer(
    SuccessorCountArray& sca) {
  /* Locals */
  unsigned int layer_number = sca.GetLayerNumber();

  /* Dont process layers without states */
  if (db.GetNumberOfKnots(layer_number) == 0) {
    return true;
  }

  /* Output & filenames */
  log << "*********************************************\n"
      << "*** Prepare count arrays for layers " << layer_number << " ***\n"
      << "*********************************************\n";

  /* Calculate number of succedding states */
  if (!CalcNumSuccedors(layer_number)) {
    return log.Log(Logger::LogLevel::error,
                   L"Could not calculate number of succeddors!");
  }

  /* Finish */
  return true;
}

/* Calculate the number of succeeding states for each state of the current
 * layer. */
bool mini_max::retro_analysis::SuccessorCountManager::CalcNumSuccedors(
    unsigned int layer_number) {
  /* Chekcs */
  if (layer_number >= db.GetNumLayers()) {
    log << "ERROR: Layer number is out of range!\n";
    return ReturnValues::FalseOrStop();
  }
  if (db.GetNumberOfKnots(layer_number) == 0) {
    return true;
  }

  log << "*** Calculate number of succeding states for each state of layer "
      << layer_number << " ***" << "\n";

  /* Mark all layers as not processed. This is important, since the db/game
   * might return the same layer multiple times as succeeding layer */
  layer_processed.resize(db.GetNumLayers(), false);

  /* Go through each state in the current layer ... */
  if (!AddNumSuccedors(layer_number)) {
    log << "ERROR: Could not calculate number of succedors for layer "
        << layer_number << "!\n";
    return ReturnValues::FalseOrStop();
  }

  /* ... and go through each state in each succeeding layer */
  for (auto& cur_succ_layer : db.GetSuccLayers(layer_number)) {
    /* Dont process layers without states */
    if (!db.GetNumberOfKnots(cur_succ_layer)) {
      continue;
    }

    /* Preload succeeding layers */
    db.SetLoadingOfFullLayerOnRead();

    /* Dont process layers that have already been processed */
    if (layer_number == cur_succ_layer) {
      continue;
    }

    log << "- Do the same for the succeeding layer " << (int)cur_succ_layer
        << " (" << game.GetOutputInformation(cur_succ_layer) << ")" << "\n";

    /* Load layer into memory */
    if (!db.LoadLayerFromFile(cur_succ_layer)) {
      log << "ERROR: Could not load layer " << cur_succ_layer
          << " from file!\n";
      return ReturnValues::FalseOrStop();
    }

    if (!AddNumSuccedors(cur_succ_layer)) {
      log << "ERROR: Could not calculate number of succedors for layer "
          << cur_succ_layer << "!\n";
      return ReturnValues::FalseOrStop();
    }
  }

  /* Everything fine */
  return true;
}

/* Add the number of succeeding states for each state of the current layer */
bool mini_max::retro_analysis::SuccessorCountManager::AddNumSuccedors(
    unsigned int layer_number) {
  /* Checks */
  if (layer_number >= db.GetNumLayers()) {
    return ReturnValues::FalseOrStop();
  }
  if (db.GetNumberOfKnots(layer_number) == 0) {
    return ReturnValues::FalseOrStop();
  }
  if (layer_processed.size() <= layer_number) {
    return ReturnValues::FalseOrStop();
  }
  if (layer_processed[layer_number]) {
    return true;
  }

  /* Prepare parameters for multi threading */
  total_num_states_processed = 0;
  rough_total_num_states_processed = 0;
  ThreadManagerClass::ThreadVarsArray<AddNumSuccedorsVars> tva(
      tm.GetNumThreads(),
      AddNumSuccedorsVars(*this, layer_number,
                          rough_total_num_states_processed));

  /* Process each state in the curren layer */
  switch (tm.ExecuteParallelLoop(
      AddNumSuccedorsThreadProc, tva.GetPointerToArray(), tva.GetSizeOfArray(),
      TM_SCHEDULE_STATIC, 0, db.GetNumberOfKnots(layer_number) - 1, 1)) {
    case TM_RETURN_VALUE_OK:
      break;
    case TM_RETURN_VALUE_EXECUTION_CANCELLED:
      log << "\n"
          << "****************************************\nMain thread: Execution "
             "cancelled by user!\n****************************************\n";
      return false;
    default:
    case TM_RETURN_VALUE_INVALID_PARAM:
    case TM_RETURN_VALUE_UNEXPECTED_ERROR:
      log << "ERROR: Unexpected error in thread manager!\n";
      return ReturnValues::FalseOrStop();
  }

  /* Reduce and delete thread specific data */
  tva.Reduce();
  if (total_num_states_processed != db.GetNumberOfKnots(layer_number)) {
    log.Log(Logger::LogLevel::error,
            L"Number of processed states is less than the number of knots in "
            L"the layer!");
    return ReturnValues::FalseOrStop();
  } else {
    log << "Number of states processed: " << total_num_states_processed << "\n";
  }

  /* Mark layer as processed */
  layer_processed[layer_number] = true;

  /* Everything fine */
  return true;
}

DWORD mini_max::retro_analysis::SuccessorCountManager::
    AddNumSuccedorsThreadProc(void* p_parameter, int64_t index) {
  /* Check parameter */
  if (p_parameter == nullptr) {
    return ReturnValues::FalseOrStop();
  }

  /* Locals */
  AddNumSuccedorsVars& ans_vars = *((AddNumSuccedorsVars*)p_parameter);
  SuccessorCountManager& scm = ans_vars.scm;
  std::vector<SuccessorCountArray*>& sca = scm.succ_count_arrays;
  Logger& log = scm.log; /* All successor count arrays have the same logger */
  database::Database& db = scm.db;
  GameInterface& game = scm.game;
  StateAddressStruct cur_state;
  TwoBit cur_state_value;

  cur_state.layer_number = ans_vars.layer_number;
  cur_state.state_number = (StateNumberVarType)index;

  /* Print status */
  ans_vars.states_processed.StateProcessed(
      log, db.GetNumberOfKnots(cur_state.layer_number),
      L"    Already processed ");

  /* Invalid state? Then don't care about it */
  if (!db.ReadKnotValueFromDatabase(cur_state.layer_number,
                                    cur_state.state_number, cur_state_value)) {
    log.Log(Logger::LogLevel::error, L"ReadKnotFromDatabase() returned false!");
    return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
  }
  if (cur_state_value == SKV_VALUE_INVALID) {
    return TM_RETURN_VALUE_OK;
  }

  /* Add solved states to list of states to process */
  if (cur_state_value == SKV_VALUE_GAME_LOST ||
      cur_state_value == SKV_VALUE_GAME_WON) {
    PlyInfoVarType num_plies;
    if (!db.ReadPlyInfoFromDatabase(cur_state.layer_number,
                                    cur_state.state_number, num_plies)) {
      log.Log(Logger::LogLevel::error,
              L"ReadPlyInfoFromDatabase() returned false!");
      return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
    }
    if (!scm.states_to_process[ans_vars.cur_thread_no].PushBack(
            cur_state, num_plies,
            db.GetNumberOfKnots(cur_state.layer_number))) {
      log.Log(Logger::LogLevel::error, L"PushBack() returned false!");
      return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
    };
  }

  /* If loaded from file, then dont process the states again. The
   * states_to_process are already filled at this point here. */
  if (scm.loaded_sca_from_file) {
    return TM_RETURN_VALUE_OK;
  }

  /* Set current selected situation */
  if (!game.SetSituation(ans_vars.cur_thread_no, cur_state.layer_number,
                         cur_state.state_number)) {
    log.Log(Logger::LogLevel::error, L"SetSituation() returned false!");
    return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
  }

  /* Get list with state numbers of predecessors */
  game.GetPredecessors(ans_vars.cur_thread_no, ans_vars.pred_vars_array);

  /* Store the predecessir states in a thread specific file */
  for (unsigned int cur_pred = 0; cur_pred < ans_vars.pred_vars_array.size();
       cur_pred++) {
    StateAddressStruct pred_state;
    pred_state.layer_number =
        ans_vars.pred_vars_array[cur_pred].pred_layer_number;
    pred_state.state_number =
        ans_vars.pred_vars_array[cur_pred].pred_state_number;
    if (!ans_vars.StorePredecessorState(pred_state)) {
      log.Log(Logger::LogLevel::error,
              L"StorePredecessorState() returned false!");
      return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
    }
  }

  /* Everything is fine */
  return TM_RETURN_VALUE_OK;
}

}  // namespace muehle
