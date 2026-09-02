#include "muehle/mini_max/alpha_beta/solver.h"

#include "muehle/mini_max/alpha_beta/init_alpha_beta_vars.h"
#include "muehle/mini_max/alpha_beta/knot_struct.h"
#include "muehle/mini_max/alpha_beta/run_alpha_beta_vars.h"
#include "muehle/mini_max/return_values.h"

namespace muehle {

/* Constructor */
mini_max::alpha_beta::Solver::Solver(Logger& log, ThreadManagerClass& tm,
                                     database::Database& db,
                                     GameInterface& game)
    : log(log),
      db(db),
      game(game),
      tm(tm),
      max_num_branches(game.GetMaxNumPossibilities()) {}

bool mini_max::alpha_beta::Solver::GetBestChoice(
    unsigned int& choice, StateInfo& info_about_choices) {
  /* checks */
  if (depth_of_full_tree == 0) {
    return log.Log(Logger::LogLevel::error, L"Depth of full tree is zero"),
           false;
  }
  if (max_num_branches == 0) {
    return log.Log(Logger::LogLevel::error, L"Max number of branches is zero"),
           false;
  }

  /* Locals */
  unsigned int layer_number = game.GetLayerNumber(0);
  KnotStruct root;
  std::vector<KnotStruct> branches;
  RunAlphaBetaVars tva1(*this, layer_number, L"");

  /* initialization */
  calc_database = false;

  /* if database is not available, use min-max algorithmn without database */
  if (!db.IsOpen() && depth_of_full_tree > 2) {
    /* create one thread for each possibility */
    std::vector<unsigned int> possibility_ids;
    game.GetPossibilities(0, possibility_ids);
    if (!possibility_ids.size()) {
      choice = 0;
      info_about_choices.choices.clear();
      info_about_choices.short_value = SKV_VALUE_INVALID;
      info_about_choices.ply_info = 0;
      info_about_choices.best_amount_of_plies = 0;
      return true;
    }
    branches.resize(possibility_ids.size());
    root.player_to_move_changed = true;
    root.possibility_ids = possibility_ids;
    root.num_possibilities = possibility_ids.size();
    root.branches = branches.data();
    ThreadManagerClass::ThreadVarsArray<RunAlphaBetaVars> tva(
        tm.GetNumThreads(), RunAlphaBetaVars(*this, layer_number, L""));
    for (unsigned int i = 0; i < tm.GetNumThreads(); i++) {
      tva.item[i].root_knot = &root;
    }

    switch (tm.ExecuteParallelLoop(MinMaxThreadProc, tva.GetPointerToArray(),
                                   tva.GetSizeOfArray(), TM_SCHEDULE_STATIC, 0,
                                   possibility_ids.size() - 1, 1)) {
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
    tva.Reduce();

    /* fill the information from the sub knots of each thread into the root knot
     */
    std::vector<unsigned int> best_branches;
    if (!root.CalcKnotValue()) {
      return log.Log(Logger::LogLevel::error, L"knot.calcKnotValue() failed"),
             ReturnValues::FalseOrStop();
    }
    if (!root.CalcPlyInfo()) {
      return log.Log(Logger::LogLevel::error, L"knot.calcPlyInfo() failed"),
             ReturnValues::FalseOrStop();
    }
    if (!root.GetBestBranchesBasedOnFloatValue(best_branches)) {
      return log.Log(Logger::LogLevel::error,
                     L"knot.getBestBranchesBasedOnFloatValue() failed"),
             ReturnValues::FalseOrStop();
    }
    unsigned int best_branch =
        (best_branches.size() ? best_branches[rand() % best_branches.size()]
                              : 0);
    root.best_move_id = root.possibility_ids[best_branch];

    /* use database, with one single thread */
  } else {
    tva1.cur_thread_no = 0;
    if (!LetTheTreeGrow(root, tva1, depth_of_full_tree, FPKV_MIN_VALUE,
                        FPKV_MAX_VALUE)) {
      return log.Log(Logger::LogLevel::error, L"letTheTreeGrow() failed"),
             false;
    }
  }

  /* calc information about choices */
  choice = root.best_move_id;
  root.GetInfoAboutChoices(info_about_choices);
  info_about_choices.UpdateBestAmountOfPlies();

  return true;
}

DWORD mini_max::alpha_beta::Solver::MinMaxThreadProc(void* p_parameter,
                                                     int64_t index) {
  /* check */
  if (p_parameter == NULL) return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;

  /* locals */
  RunAlphaBetaVars& rab_vars = *((RunAlphaBetaVars*)p_parameter);
  Solver& ab_solver = rab_vars.r_solver;
  Logger& log = ab_solver.log;
  GameInterface& game = ab_solver.game;
  void* p_backup;
  bool player_to_move_changed;

  /* perform move for this thread with the corresponding possibility */
  game.Move(rab_vars.cur_thread_no, rab_vars.root_knot->possibility_ids[index],
            player_to_move_changed, p_backup);

  /* calc value of considered possibility */
  if (!ab_solver.LetTheTreeGrow(rab_vars.root_knot->branches[index], rab_vars,
                                ab_solver.depth_of_full_tree - 1,
                                FPKV_MIN_VALUE, FPKV_MAX_VALUE)) {
    return log.Log(Logger::LogLevel::error, L"letTheTreeGrow() failed"),
           TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
  }

  /* undo move */
  game.Undo(rab_vars.cur_thread_no, rab_vars.root_knot->possibility_ids[index],
            player_to_move_changed, p_backup);

  return TM_RETURN_VALUE_OK;
}

/* return value is true if calculation is stopped either by user or by an
error */
bool mini_max::alpha_beta::Solver::CalcKnotValueByAlphaBeta(
    std::vector<unsigned int>& layers_to_calculate) {
  /* loop through all layers */
  for (auto layer_number : layers_to_calculate) {
    /* checks */
    if (layer_number >= game.GetNumberOfLayers()) {
      return log.Log(Logger::LogLevel::error, L"Layer number is out of range"),
             ReturnValues::FalseOrStop();
    }
    if (!db.IsOpen()) {
      return log.Log(Logger::LogLevel::error,
                     L"Database is not open. It must be open to store the "
                     L"calculated values."),
             ReturnValues::FalseOrStop();
    }

    /* skip if layer is already calculated */
    if (db.IsLayerCompleteAndInFile(layer_number)) {
      return log << "  Layer " << layer_number << " is already calculated"
                 << "\n",
             true;
    }

    /* enable database calculation */
    log << "\n"
        << "*** Calculate layer " << layer_number
        << " by alpha-beta-algorithmn ***" << "\n";
    cur_action = Activity::perform_alpha_beta;
    calc_database = true;
    depth_of_full_tree = game.GetMaxNumPlies() + 1;

    /* initialization */
    if (!Init(layer_number)) {
      return false;
    }

    /* preload succeeding layers */
    db.SetLoadingOfFullLayerOnRead();

    /* run alpha-beta algorithmn */
    if (!Run(layer_number)) {
      return false;
    }

    /* show stats */
    db.UpdateLayerStats(layer_number);
    db.ShowLayerStats(layer_number);
  }

  return true;
}

/* Sets the maximum search depth for the alpha-beta algorithm. */
void mini_max::alpha_beta::Solver::SetSearchDepth(
    unsigned int max_alpha_beta_search_depth) {
  depth_of_full_tree = max_alpha_beta_search_depth;
}

/* The function setSituation is called for each state to mark the invalid
ones. */
bool mini_max::alpha_beta::Solver::Init(unsigned int layer_number) {
  /* skip if layer has no knots */
  if (db.GetNumberOfKnots(layer_number) == 0) {
    log << "  Skip calculation of layer " << layer_number
        << ", since it has no knots" << "\n";
    return true;
  }

  /* locals */
  std::wstringstream ss_inv_array_directory;
  std::wstringstream ss_inv_array_file_path;
  std::wstring const file_directory = db.GetFileDirectory();

  /* set current processed layer number */
  log << "\n"
      << "*** Signing of invalid states for layer " << layer_number << " ("
      << (game.GetOutputInformation(layer_number)) << ") which has "
      << db.GetNumberOfKnots(layer_number) << " knots ***" << "\n";

  /* file names */
  ss_inv_array_directory.str(L"");
  ss_inv_array_directory << file_directory << (file_directory.size() ? "/" : "")
                         << "invalidStates";
  ss_inv_array_file_path.str(L"");
  ss_inv_array_file_path << file_directory << (file_directory.size() ? "/" : "")
                         << "invalidStates/invalidStatesOfLayer" << layer_number
                         << ".dat";

  /* does initialization file exist ? */
  CreateDirectory(ss_inv_array_directory.str().c_str(), NULL);

  /* prepare parameters */
  total_num_states_processed = 0;
  rough_total_num_states_processed = 0;
  ThreadManagerClass::ThreadVarsArray<InitAlphaBetaVars> tva(
      tm.GetNumThreads(),
      InitAlphaBetaVars(*this, layer_number, ss_inv_array_file_path.str()));

  /* process each state in the current layer */
  switch (tm.ExecuteParallelLoop(InitThreadProc, tva.GetPointerToArray(),
                                 tva.GetSizeOfArray(), TM_SCHEDULE_STATIC, 0,
                                 db.GetNumberOfKnots(layer_number) - 1, 1)) {
    case TM_RETURN_VALUE_OK:
      break;
    case TM_RETURN_VALUE_EXECUTION_CANCELLED:
      log << "\n"
          << "****************************************\nMain thread: Execution "
             "cancelled by user!\n****************************************\n"
          << "\n";
      return false;
    default:
    case TM_RETURN_VALUE_INVALID_PARAM:
    case TM_RETURN_VALUE_UNEXPECTED_ERROR:
      log << "\n"
          << "****************************************\nMain thread: Invalid "
             "or unexpected param!\n****************************************\n"
          << "\n";
      return ReturnValues::FalseOrStop();
  }

  /* reduce and delete thread specific data */
  tva.Reduce();

  /* check if all states have been processed */
  if (total_num_states_processed != db.GetNumberOfKnots(layer_number)) {
    return log.Log(
               Logger::LogLevel::error,
               L"totalNumStatesProcessed != db.getNumberOfKnots(layerNumber)"),
           ReturnValues::FalseOrStop();
  }

  /* show statistics */
  db.UpdateLayerStats(layer_number);
  db.ShowLayerStats(layer_number);

  return true;
}

/* set short knot value to SKV_VALUE_INVALID, ply info to
PLYINFO_VALUE_INVALID and knotAlreadyCalculated to true or false, whether
setSituation() returns true or false */
DWORD mini_max::alpha_beta::Solver::InitThreadProc(void* p_parameter,
                                                   int64_t index) {
  /* check */
  if (p_parameter == NULL) return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;

  /* locals */
  InitAlphaBetaVars& iab_vars = *((InitAlphaBetaVars*)p_parameter);
  Solver& ab_solver = iab_vars.r_solver;
  Logger& log = ab_solver.log;
  GameInterface& game = ab_solver.game;
  database::Database& db = ab_solver.db;
  float float_value; /* dummy variable for calls of getValueOfSituation() */
  StateAddressStruct cur_state; /* current state counter for loops */
  TwoBit cur_state_value = 0;   /* for calls of getValueOfSituation() */
  PlyInfoVarType ply_info;      /* depends on the curStateValue */

  cur_state.layer_number = iab_vars.layer_number;
  cur_state.state_number = index;

  /* print status */
  iab_vars.states_processed.StateProcessed(
      log, db.GetNumberOfKnots(cur_state.layer_number),
      L"Already initialized ");

  /* layer initialization already done ? if so, then read from file */
  if (iab_vars.load_from_file) {
    if (!iab_vars.ReadByte(cur_state.state_number * sizeof(TwoBit),
                           cur_state_value)) {
      return log.Log(Logger::LogLevel::error,
                     L"initThreadProc::readBytes failed"),
             TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
    }

    /* initialization not done */
  } else {
    /* set current selected situation */
    if (!game.SetSituation(iab_vars.cur_thread_no, cur_state.layer_number,
                           cur_state.state_number)) {
      cur_state_value = SKV_VALUE_INVALID;
    } else {
      /* get value of current situation */
      game.GetValueOfSituation(iab_vars.cur_thread_no, float_value,
                               cur_state_value);
    }
  }

  /* calc ply info */
  if (cur_state_value == SKV_VALUE_GAME_WON ||
      cur_state_value == SKV_VALUE_GAME_LOST) {
    ply_info = 0;
  } else if (cur_state_value == SKV_VALUE_INVALID) {
    ply_info = PLYINFO_VALUE_INVALID;
  } else {
    ply_info = PLYINFO_VALUE_UNCALCULATED;
  }

  /* save short knot value & ply info */
  if (!db.WriteKnotValueInDatabase(cur_state.layer_number,
                                   cur_state.state_number, cur_state_value)) {
    return log.Log(Logger::LogLevel::error,
                   L"db.writeKnotValueInDatabase() failed"),
           TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
  }
  if (!db.WritePlyInfoInDatabase(cur_state.layer_number, cur_state.state_number,
                                 ply_info)) {
    return log.Log(Logger::LogLevel::error,
                   L"db.writePlyInfoInDatabase() failed"),
           TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
  }

  /* write data to buffered file */
  if (!iab_vars.load_from_file) {
    if (!iab_vars.WriteByte(cur_state.state_number * sizeof(TwoBit),
                            cur_state_value)) {
      return log.Log(Logger::LogLevel::error,
                     L"initThreadProc writeBytes failed!"),
             TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
    }
  }

  return TM_RETURN_VALUE_OK;
}

bool mini_max::alpha_beta::Solver::Run(unsigned int layer_number) {
  /* skip if layer has no knots */
  if (db.GetNumberOfKnots(layer_number) == 0) {
    log << "Skip calculation of layer " << layer_number
        << ", since it has no knots" << "\n";
    return true;
  }

  /* prepare parameters */
  log << "\n"
      << "*** Calculate layer " << layer_number
      << " with function letTheTreeGrow(): ***" << "\n";
  total_num_states_processed = 0;
  rough_total_num_states_processed = 0;
  ThreadManagerClass::ThreadVarsArray<RunAlphaBetaVars> tva(
      tm.GetNumThreads(), RunAlphaBetaVars(*this, layer_number, L""));

  /* process each state in the current layer */
  switch (tm.ExecuteParallelLoop(RunThreadProc, tva.GetPointerToArray(),
                                 tva.GetSizeOfArray(), TM_SCHEDULE_STATIC, 0,
                                 db.GetNumberOfKnots(layer_number) - 1, 1)) {
    case TM_RETURN_VALUE_OK:
      break;
    case TM_RETURN_VALUE_EXECUTION_CANCELLED:
      log << "\n"
          << "****************************************\nMain thread: Execution "
             "cancelled by user!\n****************************************\n"
          << "\n";
      return false;
    default:
    case TM_RETURN_VALUE_INVALID_PARAM:
    case TM_RETURN_VALUE_UNEXPECTED_ERROR:
      return ReturnValues::FalseOrStop();
  }

  /* reduce and delete thread specific data */
  tva.Reduce();
  if (total_num_states_processed != db.GetNumberOfKnots(layer_number)) {
    return log.Log(
               Logger::LogLevel::error,
               L"totalNumStatesProcessed < db.getNumberOfKnots(layerNumber)"),
           ReturnValues::FalseOrStop();
  }

  /* show statistics */
  db.UpdateLayerStats(layer_number);
  db.ShowLayerStats(layer_number);

  return true;
}

DWORD mini_max::alpha_beta::Solver::RunThreadProc(void* p_parameter,
                                                  int64_t index) {
  /* check */
  if (p_parameter == NULL) return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;

  /* locals */
  RunAlphaBetaVars& rab_vars = *((RunAlphaBetaVars*)p_parameter);
  Solver& ab_solver = rab_vars.r_solver;
  Logger& log = ab_solver.log;
  database::Database& db = ab_solver.db;
  GameInterface& game = ab_solver.game;
  StateAddressStruct cur_state; /* current state counter for loops */
  KnotStruct root;              /* root knot of the tree, which is calculated */
  PlyInfoVarType
      ply_info; /* for checking if knot value is already calculated or not */

  cur_state.layer_number = rab_vars.layer_number;
  cur_state.state_number = index;

  /* print status */
  rab_vars.states_processed.StateProcessed(
      log, db.GetNumberOfKnots(cur_state.layer_number), L"  Processed ");

  /* state already calculated? if so, leave. */
  if (!db.ReadPlyInfoFromDatabase(cur_state.layer_number,
                                  cur_state.state_number, ply_info)) {
    return log.Log(Logger::LogLevel::error,
                   L"db.readPlyInfoFromDatabase() failed"),
           TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
  }
  if (ply_info != PLYINFO_VALUE_UNCALCULATED) {
    return TM_RETURN_VALUE_OK;
  }

  /* set current selected situation */
  if (game.SetSituation(rab_vars.cur_thread_no, cur_state.layer_number,
                        cur_state.state_number)) {
    /* debug print */
    if (log.GetLevel() >= Logger::LogLevel::trace) {
      log << "Calc layer: " << cur_state.layer_number
          << " state: " << cur_state.state_number << "\n";
      game.PrintField(rab_vars.cur_thread_no, SKV_VALUE_INVALID);
    }

    /* calc value of situation */
    if (!ab_solver.LetTheTreeGrow(root, rab_vars, ab_solver.depth_of_full_tree,
                                  SKV_VALUE_GAME_LOST, SKV_VALUE_GAME_WON)) {
      return log.Log(Logger::LogLevel::error, L"letTheTreeGrow() failed"),
             TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
    }

  } else {
    /* should not occur, because already tested by plyInfo == */
    /* PLYINFO_VALUE_UNCALCULATED */
    return log.Log(Logger::LogLevel::error,
                   L"This event should never occur. if (!m->setSituation())"),
           TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
  }
  return TM_RETURN_VALUE_OK;
}

bool mini_max::alpha_beta::Solver::LetTheTreeGrow(KnotStruct& knot,
                                                  RunAlphaBetaVars& rab_vars,
                                                  unsigned int til_level,
                                                  float alpha, float beta) {
  /* checks */
  if (til_level > depth_of_full_tree) {
    return log.Log(Logger::LogLevel::error, L"tilLevel > depthOfFullTree"),
           ReturnValues::FalseOrStop();
  }
  if (max_num_branches == 0) {
    return log.Log(Logger::LogLevel::error, L"Max number of branches is zero"),
           ReturnValues::FalseOrStop();
  }

  /* Locals */
  unsigned int layer_number = 0; /* layer number of current state */
  unsigned int state_number = 0; /* state number of current state */
  unsigned int max_won_freq_values_sub_moves =
      0; /* maximum number of freqValuesSubMoves[SKV_VALUE_GAME_WON] */

  /* evaluate situation, if last search depth level */
  if (til_level == 0) {
    /* If tilLevel is equal to zero while calculating the database, this */
    /* indicates that the recursion has reached its maximum depth and exhausted
     */
    /* available memory resources. Each recursive step consumes memory, so */
    /* hitting zero means the algorithm cannot continue deeper, which may result
     */
    /* in incomplete calculations or failure to store all required states. */
    /* Consider increasing available memory or optimizing the recursion depth to
     */
    /* avoid this situation during large database calculations. */
    if (calc_database) {
      knot.SetInvalid();
      return log.Log(Logger::LogLevel::error,
                     L"tilLevel == 0 while calculating database"),
             ReturnValues::FalseOrStop();
    } else {
      game.GetValueOfSituation(rab_vars.cur_thread_no, knot.float_value,
                               knot.short_value);
    }

    /* investigate branches */
  } else {
    /* standard values */
    knot.InitForCaclulation(
        &rab_vars.branch_array[(depth_of_full_tree - til_level) *
                               max_num_branches]);

    /* get layer and state number of current state and look if short knot value
     */
    /* can be found in database or in an array */
    if (TryDatabase(knot, rab_vars, til_level, layer_number, state_number))
      return true;

    /* get number of possiblities */
    game.GetPossibilities(rab_vars.cur_thread_no, knot.possibility_ids);
    knot.num_possibilities = (unsigned int)knot.possibility_ids.size();

    /* debug print */
    if (log.GetLevel() >= Logger::LogLevel::trace) {
      log << std::wstring(2 * (depth_of_full_tree - til_level), L' ')
          << "Number of move possibilities: " << knot.num_possibilities << "\n";
    }

    /* unable to move */
    if (knot.num_possibilities == 0) {
      /* if unable to move a final state is reached */
      game.GetValueOfSituation(rab_vars.cur_thread_no, knot.float_value,
                               knot.short_value);
      knot.ply_info =
          (SKV_VALUE_INVALID == knot.short_value) ? PLYINFO_VALUE_INVALID : 0;
      if (til_level == depth_of_full_tree - 1)
        knot.freq_values_sub_moves[knot.short_value]++;

      /* if unable to move an invalid state was reached if nobody has won */
      if (calc_database && game.LostIfUnableToMove(rab_vars.cur_thread_no) &&
          knot.short_value == SKV_VALUE_GAME_DRAWN) {
        knot.SetInvalid();
      }

      /* movement is possible */
    } else {
      /* move, letTreeGrow, undo */
      if (!TryPossibilities(knot, rab_vars, til_level,
                            max_won_freq_values_sub_moves, alpha, beta)) {
        return log.Log(Logger::LogLevel::error, L"tryPossibilities() failed"),
               ReturnValues::FalseOrStop();
      }

      /* calculate value of knot - its the value of the best branch */
      if (!knot.CalcKnotValue()) {
        return log.Log(Logger::LogLevel::error, L"knot.calcKnotValue() failed"),
               ReturnValues::FalseOrStop();
      }

      /* calc ply info */
      if (!knot.CalcPlyInfo()) {
        return log.Log(Logger::LogLevel::error, L"knot.calcPlyInfo() failed"),
               ReturnValues::FalseOrStop();
      }

      /* select randomly one of the best moves, if they are equivalent */
      if (til_level == depth_of_full_tree && !calc_database) {
        std::vector<unsigned int> best_branches;
        if (db.IsOpen()) {
          if (!knot.GetBestBranchesBasedOnSkvValue(best_branches)) {
            return log.Log(Logger::LogLevel::error,
                           L"knot.getBestBranchesBasedOnSkvValue() failed"),
                   ReturnValues::FalseOrStop();
          }
        } else {
          if (!knot.GetBestBranchesBasedOnFloatValue(best_branches)) {
            return log.Log(Logger::LogLevel::error,
                           L"knot.getBestBranchesBasedOnFloatValue() failed"),
                   ReturnValues::FalseOrStop();
          }
        }
        unsigned int best_branch =
            (best_branches.size() ? best_branches[rand() % best_branches.size()]
                                  : 0);
        knot.best_move_id = knot.possibility_ids[best_branch];
      } else if (!calc_database) {
        knot.best_move_id =
            (knot.possibility_ids.size() > 0) ? knot.possibility_ids[0] : 0;
      }
    }

    /* debug print */
    if (log.GetLevel() >= Logger::LogLevel::trace) {
      log << std::wstring(2 * (depth_of_full_tree - til_level), L' ')
          << "Write value of current state to database: " << knot.ply_info
          << " plies" << "\n";
      game.PrintField(rab_vars.cur_thread_no, knot.short_value,
                      2 * (depth_of_full_tree - til_level));
    }

    /* save value and best branch into database and set value as valid */
    if (calc_database && db.IsOpen()) {
      if (!SaveInDatabase(knot, rab_vars, layer_number, state_number)) {
        log << " layerNumber: " << layer_number
            << " stateNumber: " << state_number << "\n";
        log << "tilLevel: " << til_level << "\n";
        log << "depthOfFullTree: " << depth_of_full_tree << "\n";
        log << "knot.shortValue: " << knot.short_value << "\n";
        log << "knot.plyInfo: " << knot.ply_info << "\n";
        log << "knot.numPossibilities: " << knot.num_possibilities << "\n";
        return log.Log(Logger::LogLevel::error, L"saveInDatabase() failed"),
               ReturnValues::FalseOrStop();
      }
    }
  }
  return true;
}

/* 1 - Determines layerNumber and stateNumber for the given game situation.
2 - Look into database if knot value and ply info are already calculated. If
so sets knot.shortValue, knot.floatValue and knot.plyInfo. */
bool mini_max::alpha_beta::Solver::TryDatabase(KnotStruct& knot,
                                               const RunAlphaBetaVars& rab_vars,
                                               unsigned int til_level,
                                               unsigned int& layer_number,
                                               unsigned int& state_number) {
  /* locals */
  TwoBit short_knot_value = SKV_VALUE_INVALID;
  PlyInfoVarType ply_info = PLYINFO_VALUE_UNCALCULATED;
  unsigned int sym_op;

  /* use database ? */
  if (db.IsOpen() &&
      (calc_database || db.IsLayerCompleteAndInFile(rab_vars.layer_number))) {
    /* lock mutex for database access */
    std::lock_guard<std::mutex> lock(mutex);

    /* situation already existend in database ? */
    game.GetLayerAndStateNumber(rab_vars.cur_thread_no, layer_number,
                                state_number, sym_op);
    bool layer_in_database_and_completed =
        db.IsLayerCompleteAndInFile(layer_number);
    if (!db.ReadKnotValueFromDatabase(layer_number, state_number,
                                      short_knot_value)) {
      return log.Log(Logger::LogLevel::error,
                     L"db.readKnotValueFromDatabase() failed"),
             ReturnValues::FalseOrStop();
    }
    if (!db.ReadPlyInfoFromDatabase(layer_number, state_number, ply_info)) {
      return log.Log(Logger::LogLevel::error,
                     L"db.readPlyInfoFromDatabase() failed"),
             ReturnValues::FalseOrStop();
    }

    /* debug print */
    if (log.GetLevel() >= Logger::LogLevel::trace) {
      log << std::wstring(2 * (depth_of_full_tree - til_level), L' ')
          << "Current state: " << layer_number << " state: " << state_number
          << "\n";
      game.PrintField(rab_vars.cur_thread_no, short_knot_value,
                      2 * (depth_of_full_tree - til_level));
    }

    /* it was possible to achieve an invalid state using move(), so the original
     */
    /* state was an invalid one */
    if ((til_level < depth_of_full_tree &&
         short_knot_value == SKV_VALUE_INVALID &&
         layer_in_database_and_completed) ||
        (til_level < depth_of_full_tree &&
         short_knot_value == SKV_VALUE_INVALID &&
         (ply_info != PLYINFO_VALUE_UNCALCULATED &&
          ply_info != PLYINFO_VALUE_INVALID))) {
      knot.SetInvalid();
      return true;
    }

    /* print output, if not calculating database, but requesting a knot value */
    if (!calc_database && short_knot_value != SKV_VALUE_INVALID &&
        til_level == depth_of_full_tree && layer_in_database_and_completed) {
      log.Log(Logger::LogLevel::trace,
              std::wstring(L"This state is marked as ") +
                  ((short_knot_value == SKV_VALUE_GAME_WON)
                       ? L"WON"
                       : ((short_knot_value == SKV_VALUE_GAME_LOST)
                              ? L"LOST"
                              : ((short_knot_value == SKV_VALUE_GAME_DRAWN)
                                     ? L"DRAW"
                                     : L"INVALID"))));
    }

    /* when knot value is valid then return best branch */
    if ((calc_database && til_level < depth_of_full_tree &&
         short_knot_value != SKV_VALUE_INVALID &&
         ply_info != PLYINFO_VALUE_UNCALCULATED) ||
        (!calc_database && til_level < depth_of_full_tree - 1 &&
         short_knot_value != SKV_VALUE_INVALID)) {
      /* switch if is not opponent level */
      knot.short_value = short_knot_value;
      knot.ply_info = ply_info;
      knot.float_value = skv_float_value_map[knot.short_value];

      /* debug print */
      if (log.GetLevel() >= Logger::LogLevel::trace) {
        log << std::wstring(2 * (depth_of_full_tree - til_level), L' ')
            << "Reading from database was SUCCESFUL" << "\n";
      }
      return true;
    }
  }

  /* debug print */
  if (log.GetLevel() >= Logger::LogLevel::trace) {
    log << std::wstring(2 * (depth_of_full_tree - til_level), L' ')
        << "Reading from database FAILED" << "\n";
  }
  return false;
}

bool mini_max::alpha_beta::Solver::TryPossibilities(
    KnotStruct& knot, RunAlphaBetaVars& rab_vars, unsigned int til_level,
    unsigned int& max_won_freq_values_sub_moves, float& alpha, float& beta) {
  /* locals */
  void* p_backup;
  unsigned int cur_poss;

  for (cur_poss = 0; cur_poss < knot.num_possibilities; cur_poss++) {
    /* debug output */
    if (false && til_level == depth_of_full_tree && !calc_database) {
      game.PrintMoveInformation(rab_vars.cur_thread_no,
                                knot.possibility_ids[cur_poss]);
    }

    /* move */
    game.Move(rab_vars.cur_thread_no, knot.possibility_ids[cur_poss],
              knot.branches[cur_poss].player_to_move_changed, p_backup);

    /* debug print */
    if (log.GetLevel() >= Logger::LogLevel::trace) {
      log << std::wstring(2 * (depth_of_full_tree - til_level), L' ')
          << "Moved according to possiblity " << cur_poss;
    }

    /* recursive call */
    if (!LetTheTreeGrow(knot.branches[cur_poss], rab_vars, til_level - 1, alpha,
                        beta)) {
      return log.Log(Logger::LogLevel::error, L"letTheTreeGrow() failed"),
             ReturnValues::FalseOrStop();
    }

    /* undo move */
    game.Undo(rab_vars.cur_thread_no, knot.possibility_ids[cur_poss],
              knot.branches[cur_poss].player_to_move_changed, p_backup);

    /* debug print */
    if (log.GetLevel() >= Logger::LogLevel::trace) {
      log << std::wstring(2 * (depth_of_full_tree - til_level), L' ')
          << "Last move was undone" << "\n";
    }

    /* output */
    if (til_level == depth_of_full_tree && !calc_database) {
      if (knot.freq_values_sub_moves[SKV_VALUE_GAME_WON] >
              max_won_freq_values_sub_moves &&
          knot.branches[cur_poss].short_value == SKV_VALUE_GAME_DRAWN) {
        max_won_freq_values_sub_moves =
            knot.freq_values_sub_moves[SKV_VALUE_GAME_WON];
      }
      if (db.IsOpen()) {
        knot.IncreaseFreqValuesSubMoves(cur_poss);
      }
    } else if (til_level == depth_of_full_tree - 1 && !calc_database) {
      knot.IncreaseFreqValuesSubMoves(cur_poss);
    }

    /* don't use cutting off if we are calculating the database, since we want
     */
    /* to calculate all states */
    if (db.IsOpen() && calc_database) continue;
    if (db.IsOpen() && til_level + 1 >= depth_of_full_tree) continue;

    /* check if we can spare the other possibilities according to alpha beta */
    /* algorithmn */
    if (knot.CanCutOff(cur_poss, alpha, beta)) break;
  }

  return true;
}

bool mini_max::alpha_beta::Solver::SaveInDatabase(const KnotStruct& knot,
                                                  RunAlphaBetaVars& rab_vars,
                                                  unsigned int layer_number,
                                                  unsigned int state_number) {
  /* locals */
  StateAddressStruct sym_state;
  unsigned int i;

  /* invalid value? */
  if (knot.short_value > SKV_VALUE_GAME_WON) {
    return log.Log(Logger::LogLevel::error,
                   L"alphaBeta::solver::saveInDatabase(): knot.shortValue > "
                   L"SKV_VALUE_GAME_WON"),
           ReturnValues::FalseOrStop();
  }

  /* get numbers of symmetric states */
  game.GetSymStateNumWithDuplicates(rab_vars.cur_thread_no,
                                    rab_vars.sym_states);

  /* lock mutex for database access */
  std::lock_guard<std::mutex> lock(mutex);

  /* save */
  if (!db.WriteKnotValueInDatabase(layer_number, state_number,
                                   knot.short_value)) {
    return log.Log(Logger::LogLevel::error,
                   L"db.writeKnotValueInDatabase() failed"),
           ReturnValues::FalseOrStop();
  }
  if (!db.WritePlyInfoInDatabase(layer_number, state_number, knot.ply_info)) {
    return log.Log(Logger::LogLevel::error,
                   L"db.writePlyInfoInDatabase() failed"),
           ReturnValues::FalseOrStop();
  }

  /* save value for all symmetric states */
  for (i = 0; i < rab_vars.sym_states.size(); i++) {
    /* get state number */
    sym_state = rab_vars.sym_states[i];

    /* don't save original state twice */
    if (sym_state.layer_number == layer_number &&
        sym_state.state_number == state_number)
      continue;

    /* don't save states of completed layers */
    if (db.IsLayerCompleteAndInFile(sym_state.layer_number)) continue;

    /* save */
    if (!db.WriteKnotValueInDatabase(
            sym_state.layer_number, sym_state.state_number, knot.short_value)) {
      return log.Log(Logger::LogLevel::error,
                     L"db.writeKnotValueInDatabase() failed"),
             ReturnValues::FalseOrStop();
    }
    if (!db.WritePlyInfoInDatabase(sym_state.layer_number,
                                   sym_state.state_number, knot.ply_info)) {
      return log.Log(Logger::LogLevel::error,
                     L"db.writePlyInfoInDatabase() failed"),
             ReturnValues::FalseOrStop();
    }
  }

  return true;
}

} /* namespace muehle */
