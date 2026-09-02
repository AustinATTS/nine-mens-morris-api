#include "muehle/mini_max/retro_analysis/solver.h"

#include "muehle/mini_max/retro_analysis/init_retro_analysis_vars.h"
#include "muehle/mini_max/return_values.h"

namespace muehle {

/* Constructor */
mini_max::retro_analysis::Solver::Solver(Logger& log,
                                                 ThreadManagerClass& tm,
                                                 database::Database& db,
                                                 GameInterface game)
    : log(log),
      db(db),
      game(game),
      tm(tm),
      scm(log, tm, db, game, states_to_process) {}

/* Destructor */
mini_max::retro_analysis::Solver::~Solver() {}

/* Estimate total number of knots */
size_t mini_max::retro_analysis::Solver::EstimateTotalNumberOfKnots() {
  std::vector<unsigned int> layers_considered;
  size_t total_number_of_knots = 0;

  // consider all layers to calculate
  for (auto layer_number : layers_to_calculate) {
    // only consider layers which are not already considered
    if (std::find(layers_considered.begin(), layers_considered.end(),
                  layer_number) != layers_considered.end())
      continue;
    total_number_of_knots += db.GetNumberOfKnots(layer_number);
    layers_considered.push_back(layer_number);

    // consider all successor layers of the current layer
    for (auto& cur_succ_layer : db.GetSuccLayers(layer_number)) {
      if (std::find(layers_considered.begin(), layers_considered.end(),
                    cur_succ_layer) != layers_considered.end())
        continue;
      total_number_of_knots += db.GetNumberOfKnots(cur_succ_layer);
      layers_considered.push_back(cur_succ_layer);
    }
  }
  return total_number_of_knots;
}

/* The COUNT-ARRAY is the main element of the algorithmn. It contains the number
of succeding states for the drawn gamestates, whose short knot value has to
be determined. If all succeding states (branches representing possible moves)
are for example won than, a state can be marked as lost, since no branch will
lead to a drawn or won situation any more. Each time the short knot value of
a game state has been determined, the state will be added to
'states_to_process'. This list is like a queue of states, which still has to be
processed. */
bool mini_max::retro_analysis::Solver::CalcKnotValuesByRetroAnalysis(
    std::vector<unsigned int>& layers_to_calculate) {
  // check if database is open
  if (!db.IsOpen()) {
    log << "ERROR: Database is not open!\n";
    return ReturnValues::FalseOrStop();
  }
  // check if there are layers to calculate
  if (layers_to_calculate.size() == 0) {
    log << "ERROR: No layers to calculate!\n";
    return ReturnValues::FalseOrStop();
  }
  // check if any layer to calculate has a bigger index than the number of
  // layers in the database
  for (auto layer_number : layers_to_calculate) {
    if (layer_number >= db.GetNumLayers()) {
      log << "ERROR: Layer number " << layer_number << " is out of range!\n";
      return ReturnValues::FalseOrStop();
    }
  }

  // init
  this->layers_to_calculate = layers_to_calculate;
  std::wstringstream ss_layers;
  size_t total_number_of_knots = EstimateTotalNumberOfKnots();

  states_to_process.clear();
  states_to_process.reserve(tm.GetNumThreads());
  for (unsigned int thread_no = 0; thread_no < tm.GetNumThreads();
       thread_no++) {
    states_to_process.push_back(
        StateQueue(log, db.GetFileDirectory(), thread_no));

    // resize state queue for each thread
    for (unsigned int ply_number = 0; ply_number < PLYINFO_EXP_VALUE;
         ply_number++) {
      states_to_process[thread_no].Resize(ply_number, total_number_of_knots);
    }
  }
  layer_initialized.resize(db.GetNumLayers(), false);

  // stdout
  for (auto layer_number : layers_to_calculate)
    ss_layers << " " << layer_number;
  log << "==================================================================\n"
      << "=== Calculate layers" << ss_layers.str() << " by retro analysis ===\n"
      << "==================================================================\n";

  // initialization
  log << "Bytes in memory: " << db.GetMemoryUsed() << "\n";
  if (!InitRetroAnalysis()) {
    log << "ERROR: Could not initialize retro analysis!\n";
    return ReturnValues::FalseOrStop();
  }

  // prepare count arrays
  log << "Bytes in memory: " << db.GetMemoryUsed() << "\n";
  if (!PrepareCountArrays()) {
    log << "ERROR: Could not prepare count arrays!\n";
    return ReturnValues::FalseOrStop();
  }

  // iteration
  log << "Bytes in memory: " << db.GetMemoryUsed() << "\n";
  if (!PerformRetroAnalysis()) {
    log << "ERROR: Could not perform retro analysis!\n";
    return ReturnValues::FalseOrStop();
  }

  // show output
  log << "Bytes in memory: " << db.GetMemoryUsed() << "\n";
  for (auto layer_number : layers_to_calculate) {
    db.UpdateLayerStats(layer_number);
    db.ShowLayerStats(layer_number);
  }
  log << "\n";

  // everything fine
  return true;
}

/* The state values for all game situations in the database are marked as
invalid, as undecided, as won or as lost by using the function
getValueOfSituation(). */
bool mini_max::retro_analysis::Solver::InitRetroAnalysis() {
  // locals
  std::wstringstream ss_init_array_path;  // path of the working directory
  std::wstringstream
      ss_init_array_file_path;  // filename corresponding to a cyclic
                                // array file which is used for storage
  std::wstring const file_directory = db.GetFileDirectory();

  // process each layer
  for (auto layer_number : layers_to_calculate) {
    // ensure that layer has any knots
    if (!db.GetNumberOfKnots(layer_number)) {
      log.Log(Logger::LogLevel::debug, L"Skip initialization of layer " +
                                            std::to_wstring(layer_number) +
                                            L" since it has no knots.");
      continue;
    }

    // set current processed layer number
    cur_action = Activity::init_retro_analysis;
    log << "*****************************************\n"
        << "*** Initialization of layer " << layer_number << " ("
        << (game.GetOutputInformation(layer_number)) << ") which has "
        << db.GetNumberOfKnots(layer_number) << " knots ***\n"
        << "*****************************************\n";

    // file names
    ss_init_array_path.str(L"");
    ss_init_array_path << file_directory << (file_directory.size() ? "/" : "")
                       << "initLayer";
    ss_init_array_file_path.str(L"");
    ss_init_array_file_path << file_directory
                            << (file_directory.size() ? "/" : "")
                            << "initLayer/initLayer" << layer_number << ".dat";

    // does initialization file exist ?
    CreateDirectory(ss_init_array_path.str().c_str(), NULL);

    // don't add layers twice
    if (layer_initialized[layer_number]) {
      log.Log(
          Logger::LogLevel::debug,
          L"Layer " + std::to_wstring(layer_number) + L" already initialized.");
      continue;
    }
    layer_initialized[layer_number] = true;

    // prepare parameters
    total_num_states_processed = 0;
    rough_total_num_states_processed = 0;
    ThreadManagerClass::ThreadVarsArray<InitRetroAnalysisVars> tva(
        tm.GetNumThreads(),
        InitRetroAnalysisVars(*this, layer_number,
                              ss_init_array_file_path.str()));

    // process each state in the current layer
    switch (tm.ExecuteParallelLoop(InitRetroAnalysisThreadProc,
                                   tva.GetPointerToArray(),
                                   tva.GetSizeOfArray(), TM_SCHEDULE_STATIC, 0,
                                   db.GetNumberOfKnots(layer_number) - 1, 1)) {
      case TM_RETURN_VALUE_OK:
        break;
      case TM_RETURN_VALUE_EXECUTION_CANCELLED:
        log << "\n"
            << "****************************************\nMain thread: "
               "Execution cancelled by "
               "user!\n****************************************\n";
        return false;
      default:
      case TM_RETURN_VALUE_INVALID_PARAM:
      case TM_RETURN_VALUE_UNEXPECTED_ERROR:
        return ReturnValues::FalseOrStop();
    }

    // reduce and delete thread specific data
    tva.Reduce();

    // check if all states have been processed
    if (total_num_states_processed != db.GetNumberOfKnots(layer_number)) {
      return log.Log(Logger::LogLevel::error,
                     L"Number of processed states is less than the number of "
                     L"knots in the layer!"),
             ReturnValues::FalseOrStop();
    }

    // show statistics
    db.UpdateLayerStats(layer_number);
    db.ShowLayerStats(layer_number);
  }
  return true;
}

/* Static thread procedure for initialization */
DWORD mini_max::retro_analysis::Solver::InitRetroAnalysisThreadProc(
    void* p_parameter, int64_t index) {
  // check parameter
  if (p_parameter == NULL) return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;

  // locals
  InitRetroAnalysisVars& ira_vars = *((InitRetroAnalysisVars*)p_parameter);
  Solver& retro_vars = ira_vars.retro_vars;
  Logger& log = retro_vars.log;
  GameInterface& game = retro_vars.game;
  database::Database& db = retro_vars.db;
  float float_value;  // dummy variable for calls of getValueOfSituation()
  StateAddressStruct cur_state;  // current state counter for loops
  TwoBit cur_state_value;        // for calls of getValueOfSituation()

  cur_state.layer_number = ira_vars.layer_number;
  cur_state.state_number = index;

  // print status
  ira_vars.states_processed.StateProcessed(
      log, db.GetNumberOfKnots(cur_state.layer_number),
      L"Already initialized ");

  // layer initialization already done ? if so, then read from buffered file
  if (ira_vars.load_from_file) {
    if (!ira_vars.ReadByte(cur_state.state_number * sizeof(TwoBit),
                           cur_state_value)) {
      return log.Log(Logger::LogLevel::error,
                     L"initRetroAnalysisVars::readBytes() failed"),
             TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
    }

    // initialization not done
  } else {
    // set current selected situation
    if (!game.SetSituation(ira_vars.cur_thread_no, cur_state.layer_number,
                           cur_state.state_number)) {
      cur_state_value = SKV_VALUE_INVALID;
    } else {
      // get value of current situation
      game.GetValueOfSituation(ira_vars.cur_thread_no, float_value,
                               cur_state_value);
    }
  }

  // save init value
  if (cur_state_value != SKV_VALUE_INVALID) {
    // save short knot value
    if (!db.WriteKnotValueInDatabase(cur_state.layer_number,
                                     cur_state.state_number, cur_state_value)) {
      return log.Log(Logger::LogLevel::error,
                     L"writeKnotValueInDatabase() returned false!"),
             TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
    }

    // put in queue if state is final
    if (cur_state_value == SKV_VALUE_GAME_WON ||
        cur_state_value == SKV_VALUE_GAME_LOST) {
      // ply info
      if (!db.WritePlyInfoInDatabase(cur_state.layer_number,
                                     cur_state.state_number, 0)) {
        return log.Log(Logger::LogLevel::error,
                       L"writePlyInfoInDatabase() returned false!"),
               TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
      }
    }
  }

  // write data to buffered file
  if (!ira_vars.load_from_file) {
    if (!ira_vars.WriteByte(cur_state.state_number * sizeof(TwoBit),
                            cur_state_value)) {
      log << "ERROR: initRetroAnalysisVars::writeBytes() failed!" << "\n";
      return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
    }
  }

  return TM_RETURN_VALUE_OK;
}

/* Prepare count arrays */
bool mini_max::retro_analysis::Solver::PrepareCountArrays() {
  if (!scm.Init(layers_to_calculate)) {
    log << "ERROR: Could not initialize successor count arrays!\n";
    return ReturnValues::FalseOrStop();
  }
  return true;
}

/* Perform retro analysis */
bool mini_max::retro_analysis::Solver::PerformRetroAnalysis() {
  // checks
  if (layers_to_calculate.size() == 0) {
    log << "ERROR: No layers to calculate!\n";
    return ReturnValues::FalseOrStop();
  }
  if (states_to_process.size() != tm.GetNumThreads()) {
    log << "ERROR: Number of threads and number of state queues do not "
           "match!\n";
    return ReturnValues::FalseOrStop();
  }
  if (!scm.IsReady()) {
    log << "ERROR: Number of layers to calculate and number of count arrays do "
           "not match!\n";
    return ReturnValues::FalseOrStop();
  }
  if (db.GetNumLayers() != layer_initialized.size()) {
    log << "ERROR: Number of layers and number of layer initialized flags do "
           "not match!\n";
    return ReturnValues::FalseOrStop();
  }

  // locals
  StateAddressStruct cur_state;  // current state counter for loops
  TwoBit cur_state_value;        // current state value

  log << "******************************************\n"
      << "*** Begin Iteration for Retro Analysis ***\n"
      << "******************************************\n";
  total_num_states_processed = 0;
  cur_action = Activity::perform_retro_analysis;

  db.SetLoadingOfFullLayerOnRead();

  // process each state in the current layer
  switch (
      tm.ExecuteInParallel(PerformRetroAnalysisThreadProc, (void**)this, 0)) {
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
      return ReturnValues::FalseOrStop();
  }

  // if there are still states to process, than something went wrong
  for (auto& queue : states_to_process) {
    if (queue.GetNumStatesToProcess()) {
      log << "ERROR: There are still states to process after performing retro "
             "analysis!"
          << "\n";
      return ReturnValues::FalseOrStop();
    }
  }

  // copy drawn and invalid states to ply info
  log << "    Copy drawn and invalid states to ply info database..." << "\n";
  for (auto layer_number : layers_to_calculate) {
    for (cur_state.layer_number = layer_number, cur_state.state_number = 0;
         cur_state.state_number < db.GetNumberOfKnots(cur_state.layer_number);
         cur_state.state_number++) {
      // get value of current situation
      if (!db.ReadKnotValueFromDatabase(cur_state.layer_number,
                                        cur_state.state_number,
                                        cur_state_value)) {
        log.Log(Logger::LogLevel::error,
                L"readKnotValueFromDatabase() returned false!");
        return ReturnValues::FalseOrStop();
      }
      // store ply info for drawn and invalid states
      if (cur_state_value == SKV_VALUE_GAME_DRAWN) {
        std::vector<unsigned int> possibility_ids;
        game.SetSituation(0, cur_state.layer_number, cur_state.state_number);
        game.GetPossibilities(0, possibility_ids);
        PlyInfoVarType cur_ply_value =
            (possibility_ids.size() > 0 ? PLYINFO_VALUE_DRAWN : 0);
        if (!db.WritePlyInfoInDatabase(cur_state.layer_number,
                                       cur_state.state_number, cur_ply_value)) {
          log.Log(Logger::LogLevel::error,
                  L"writePlyInfoInDatabase() returned false!");
          return ReturnValues::FalseOrStop();
        }
      }
      if (cur_state_value == SKV_VALUE_INVALID) {
        if (!db.WritePlyInfoInDatabase(cur_state.layer_number,
                                       cur_state.state_number,
                                       PLYINFO_VALUE_INVALID)) {
          log.Log(Logger::LogLevel::error,
                  L"writePlyInfoInDatabase() returned false!");
          return ReturnValues::FalseOrStop();
        }
      }
    }
  }
  log << "\n" << "*** Iteration finished! ***" << "\n";

  // every thing ok
  return true;
}

/* Static thread procedure for performing retro analysis */
DWORD mini_max::retro_analysis::Solver::PerformRetroAnalysisThreadProc(
    void* p_parameter) {
  // check parameter
  if (p_parameter == NULL) return TM_RETURN_VALUE_INVALID_PARAM;

  // locals
  Solver& retro_vars = *((Solver*)p_parameter);
  Logger& log = retro_vars.log;
  GameInterface& game = retro_vars.game;
  ThreadManagerClass& tm = retro_vars.tm;
  unsigned int thread_no = tm.GetThreadNumber();
  std::vector<PredVars> pred_vars{MAX_NUM_PREDECESSORS};

  // checks
  if (thread_no >= retro_vars.states_to_process.size()) {
    log.Log(Logger::LogLevel::error, L"Thread number is out of range! ");
    return TM_RETURN_VALUE_INVALID_PARAM;
  }

  // more locals
  StateQueue& queue = retro_vars.states_to_process[thread_no];
  long long num_states_processed =
      0;  // number of states already processed by this thread
  PlyInfoVarType cur_num_plies = 0;  // current number of plies considered
  StateAddressStruct cur_state;      // current state counter for while-loop

  // iterate through all states in the queue, ply by ply
  // IMPORTANT: All threads must process all plies, since the barrier below
  // expects all threads
  for (cur_num_plies = 0; cur_num_plies < PLYINFO_EXP_VALUE; cur_num_plies++) {
    // IMPORTANT: Since the barrier below expects all threads we cannot skip
    // here if (!queue.size(cur_num_plies)) continue;

    // process all states in the queue
    while (queue.PopFront(cur_state, cur_num_plies)) {
      // execution cancelled by user?
      if (tm.WasExecutionCancelled()) {
        log << "\n"
            << "****************************************\nSub-thread no. "
            << thread_no
            << ": Execution cancelled by "
               "user!\n****************************************\n";
        return TM_RETURN_VALUE_EXECUTION_CANCELLED;
      }

      // console output
      if (num_states_processed % OUTPUT_EVERY_N_STATES == 0) {
        std::wstringstream ss;
        ss << "    Current number of plies: " << (unsigned int)cur_num_plies
           << "/" << queue.GetMaxPlyInfoValue()
           << "      States to process for thread " << thread_no << ": "
           << queue.GetNumStatesToProcess();
        log.Log(Logger::LogLevel::info, ss.str());
      }
      num_states_processed++;

      // set current selected situation
      if (!game.SetSituation(thread_no, cur_state.layer_number,
                             cur_state.state_number)) {
        log.Log(Logger::LogLevel::error, L"No database file open!");
        return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
      }

      // DEBUGGING
      // if (cur_state.layer_number == 65 && cur_state.state_number == 17961264)
      // { 	game.PrintField(thread_no, SKV_VALUE_INVALID, 0);
      // }

      // get list with statenumbers of predecessors
      pred_vars.clear();
      game.GetPredecessors(thread_no, pred_vars);

      // iteration
      for (auto& pred_state : pred_vars) {
        if (!retro_vars.ProcessPredecessor(queue, cur_state, pred_state)) {
          log.Log(Logger::LogLevel::error,
                  L"processPredecessor() returned false!");
          log.Log(Logger::LogLevel::error,
                  L"Thread no. " + std::to_wstring(thread_no) + L" Layer: " +
                      std::to_wstring(cur_state.layer_number) + L" State: " +
                      std::to_wstring(cur_state.state_number));
          return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
        }
      }
    }

    // there might be other threads still processing states with this ply number
    tm.WaitForOtherThreads();
  }

  // every thing ok
  return TM_RETURN_VALUE_OK;
}

/* Process predecessor */
bool mini_max::retro_analysis::Solver::ProcessPredecessor(
    StateQueue& queue, const StateAddressStruct& cur_state,
    const PredVars& pred_vars_state) {
  // locals
  TwoBit cur_state_value;  // value of the current state
  PlyInfoVarType
      cur_num_plies;  // number of plies of the current considered state
  StateAddressStruct pred_state;             // predecessor state
  TwoBit pred_state_value;                   // value of the predecessor state
  PlyInfoVarType num_plies_till_pred_state;  // number of plies of the current
                                             // considered predecessor state
  static std::mutex db_mutex;                // mutex for database access

  // current predecessor
  pred_state.layer_number = pred_vars_state.pred_layer_number;
  pred_state.state_number = pred_vars_state.pred_state_number;

  // DEBUGGING
  bool v = false;
  // if (pred_state.layer_number == 65 && pred_state.state_number == 17961264) {
  // 	v = true;
  // 	if (v) log.Log(Logger::LogLevel::info, L"---------------------");
  // }

  // only states from a layer which is to be calculated are relevant
  if (pred_state.layer_number >= db.GetNumLayers() ||
      std::find(layers_to_calculate.begin(), layers_to_calculate.end(),
                pred_state.layer_number) == layers_to_calculate.end()) {
    if (v)
      log.Log(Logger::LogLevel::info,
              L"Skipping irrelevant state: Layer " +
                  std::to_wstring(pred_state.layer_number) + L", State " +
                  std::to_wstring(pred_state.state_number));
    return true;
  }

  // mutex for db
  std::lock_guard<std::mutex> lock(db_mutex);

  // get value of predecessor
  if (!db.ReadKnotValueFromDatabase(
          pred_state.layer_number, pred_state.state_number, pred_state_value)) {
    log.Log(Logger::LogLevel::error,
            L"readKnotValueFromDatabase() returned false!");
    return false;
  }

  // only drawn states are relevant here, since the others are already
  // calculated
  if (pred_state_value == SKV_VALUE_GAME_DRAWN) {
    // get value and plyInfo of current state
    if (!db.ReadKnotValueFromDatabase(
            cur_state.layer_number, cur_state.state_number, cur_state_value)) {
      log.Log(Logger::LogLevel::error,
              L"readKnotValueFromDatabase() returned false!");
      return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
    }
    if (!db.ReadPlyInfoFromDatabase(cur_state.layer_number,
                                    cur_state.state_number, cur_num_plies)) {
      log.Log(Logger::LogLevel::error,
              L"readPlyInfoFromDatabase() returned false!");
      return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
    }

    if (v)
      log.Log(Logger::LogLevel::info,
              L"ThreadId " + std::to_wstring(tm.GetThreadNumber()) +
                  L": Current state is drawn: Layer " +
                  std::to_wstring(cur_state.layer_number) + L", State " +
                  std::to_wstring(cur_state.state_number));
    if (v)
      log.Log(Logger::LogLevel::info,
              L"ThreadId " + std::to_wstring(tm.GetThreadNumber()) +
                  L": Current state value: " +
                  std::to_wstring(cur_state_value));
    if (v)
      log.Log(Logger::LogLevel::info,
              L"ThreadId " + std::to_wstring(tm.GetThreadNumber()) +
                  L": Current number of plies: " +
                  std::to_wstring(cur_num_plies));
    if (v) game.PrintField(tm.GetThreadNumber(), cur_state_value, 0);

    // if current considered state is a lost game then all predecessors are a
    // won game, if the player to move has changed
    if (cur_state_value ==
        skv_perspective_matrix[SKV_VALUE_GAME_WON]
                              [pred_vars_state.player_to_move_changed
                                   ? PL_TO_MOVE_CHANGED
                                   : PL_TO_MOVE_UNCHANGED]) {
      if (!db.WriteKnotValueInDatabase(pred_state.layer_number,
                                       pred_state.state_number,
                                       SKV_VALUE_GAME_WON)) {
        log.Log(Logger::LogLevel::error,
                L"writeKnotValueInDatabase() returned false!");
        return false;
      }
      if (!db.WritePlyInfoInDatabase(pred_state.layer_number,
                                     pred_state.state_number,
                                     cur_num_plies + 1)) {
        log.Log(Logger::LogLevel::error,
                L"writePlyInfoInDatabase() returned false!");
        return false;
      }
      // add state to queue
      if (!queue.PushBack(pred_state, cur_num_plies + 1,
                           db.GetNumberOfKnots(pred_state.layer_number))) {
        log.Log(Logger::LogLevel::error, L"push_back() returned false!");
        return false;
      }
      if (v)
        log.Log(Logger::LogLevel::info,
                L"ThreadId " + std::to_wstring(tm.GetThreadNumber()) +
                    L": Current state is a won game with " +
                    std::to_wstring(cur_num_plies + 1) + L" plies.");
      // if current state is a won game, then this state is not an option any
      // more for all predecessors
    } else {
      // reduce count value by one
      CountArrayVarType count_value = scm.GetAndDecreaseCounter(
          pred_state.layer_number, pred_state.state_number);
      if (count_value == COUNT_ARRAY_MAX_VALUE) {
        log.Log(Logger::LogLevel::error, L"Counter is at minimum value!");
        log.Log(Logger::LogLevel::error,
                L"Layer: " + std::to_wstring(pred_state.layer_number) +
                    L" State: " + std::to_wstring(pred_state.state_number));
        log.Log(Logger::LogLevel::error,
                L"Count value: " + std::to_wstring(count_value));
        return false;
      }
      if (v)
        log.Log(Logger::LogLevel::info,
                L"ThreadId " + std::to_wstring(tm.GetThreadNumber()) +
                    L": Reduced count value of predecessor to " +
                    std::to_wstring(count_value));

      // ply info
      if (!db.ReadPlyInfoFromDatabase(pred_state.layer_number,
                                      pred_state.state_number,
                                      num_plies_till_pred_state)) {
        log.Log(Logger::LogLevel::error,
                L"readPlyInfoFromDatabase() returned false!");
        return false;
      }
      if (v)
        log.Log(Logger::LogLevel::info,
                L"ThreadId " + std::to_wstring(tm.GetThreadNumber()) +
                    L": Current number of plies to pred state: " +
                    std::to_wstring(num_plies_till_pred_state));
      // write ply info, if not already done
      if (num_plies_till_pred_state == PLYINFO_VALUE_UNCALCULATED ||
          cur_num_plies + 1 > num_plies_till_pred_state) {
        if (!db.WritePlyInfoInDatabase(pred_state.layer_number,
                                       pred_state.state_number,
                                       cur_num_plies + 1)) {
          log.Log(Logger::LogLevel::error,
                  L"writePlyInfoInDatabase() returned false!");
          return false;
        }
        if (v)
          log.Log(Logger::LogLevel::info,
                  L"ThreadId " + std::to_wstring(tm.GetThreadNumber()) +
                      L": Updated ply info for predecessor to " +
                      std::to_wstring(cur_num_plies + 1));
      }

      // when all successor are won states then this is a lost state (this
      // should only be the case for one thread)
      if (count_value == 0) {
        if (!db.WriteKnotValueInDatabase(pred_state.layer_number,
                                         pred_state.state_number,
                                         SKV_VALUE_GAME_LOST)) {
          log.Log(Logger::LogLevel::error,
                  L"writeKnotValueInDatabase() returned false!");
          return false;
        }
        if (!queue.PushBack(pred_state, cur_num_plies + 1,
                             db.GetNumberOfKnots(pred_state.layer_number))) {
          log.Log(Logger::LogLevel::error, L"push_back() returned false!");
          return false;
        }
        if (v)
          log.Log(Logger::LogLevel::info,
                  L"ThreadId " + std::to_wstring(tm.GetThreadNumber()) +
                      L": Current state is a lost game with " +
                      std::to_wstring(cur_num_plies + 1) + L" plies.");
      }
    }
  }

  // everything fine
  return true;
}

}  // namespace muehle
