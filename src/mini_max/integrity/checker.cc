#include "muehle/mini_max/integrity/checker.h"

#include "muehle/mini_max/alpha_beta/knot_struct.h"
#include "muehle/mini_max/integrity/checker_thread_vars.h"
#include "muehle/mini_max/return_values.h"

namespace muehle {
/* Returns the number of states to skip during a test, depending on
   'max_num_states_to_test' setting.
   If it is not a full test then at maximum 'max_num_states_to_test' states
   are tested. */
unsigned int mini_max::integrity::Checker::GetIncrement(
    unsigned int layer_number) {
  unsigned int num_knots = db.GetNumberOfKnots(layer_number);
  if (max_num_states_to_test == 0) {
    return 1;
  }
  if (num_knots < max_num_states_to_test) {
    return 1;
  }
  return num_knots / max_num_states_to_test;
}

mini_max::integrity::Checker::Checker(Logger& log, ThreadManagerClass& tm,
                                      database::Database& db,
                                      GameInterface& game)
    : log(log),
      tm(tm),
      db(db),
      game(game),
      max_num_branches(game.GetMaxNumPossibilities()) {}

bool mini_max::integrity::Checker::StartTestThreads(
    unsigned int layer_number,
    DWORD thread_prc(void* p_parameter, int64_t index)) {
  /* Locals */
  unsigned int cur_thread_no;
  unsigned int return_value;
  unsigned int increment;
  unsigned int num_knots_in_layer = game.GetNumberOfKnotsInLayer(layer_number);

  /* output */
  log << "\n"
      << "*** Test each state in layer: " << layer_number << " ***" << "\n";
  log << (game.GetOutputInformation(layer_number)) << "\n";

  /* if nothing to test, quit */
  if (num_knots_in_layer == 0) {
    return true;
  }

  /* For debugging */
  /* tm.SetNumThreads(1); */

  game.PrepareCalculation();

  /* prepare thread specific data */
  cur_action = Activity::testing_layer;
  num_states_processed = 0;
  rough_total_num_states_processed = 0;
  ThreadManagerClass::ThreadVarsArray<CheckerThreadVars> tva(
      tm.GetNumThreads(),
      /* master: */ CheckerThreadVars(*this, layer_number, max_num_branches,
                                      rough_total_num_states_processed));

  /* Get increment and ensure it is at least 1 to avoid division by zero */
  increment = GetIncrement(layer_number);
  if (increment == 0) {
    increment = 1;
  }
  /* process each state in the current layer */
  return_value = tm.ExecuteParallelLoop(
      thread_prc, /* p_parameter: */ tva.GetPointerToArray(),
      tva.GetSizeOfArray(),
      /* schedule_type: */ TM_SCHEDULE_STATIC, /* initial_value: */ 0,
      /* final_value: */ num_knots_in_layer - 1, increment);
  switch (return_value) {
    case TM_RETURN_VALUE_OK:
    case TM_RETURN_VALUE_EXECUTION_CANCELLED:
      if (return_value == TM_RETURN_VALUE_EXECUTION_CANCELLED) {
        log << "Main thread: Execution cancelled by user" << "\n";
        return false; /* ... better would be to return a cancel-specific value
                       */
      } else {
        break;
      }
    default:
    case TM_RETURN_VALUE_INVALID_PARAM:
      log << "Main thread: TM_RETURN_VALUE_INVALID_PARAM. Execution stopped."
          << "\n";
      return ReturnValues::FalseOrStop();
    case TM_RETURN_VALUE_UNEXPECTED_ERROR:
      log << "Main thread: Unexpected error. Execution stopped." << "\n";
      return ReturnValues::FalseOrStop();
  }

  /* calculate the number of states processed */
  tva.Reduce();
  unsigned int num_states_to_process = (num_knots_in_layer - 1) / increment + 1;

  /* layer is not ok */
  if (num_states_processed < num_states_to_process) {
    log << "DATABASE ERROR IN LAYER " << layer_number << "\n";
    log << "Number of states processed: " << num_states_processed << "\n";
    log << "Number of states to process: " << num_states_to_process << "\n";
    return ReturnValues::FalseOrStop();
    /* layer is ok */
  } else {
    log << " TEST PASSED !" << "\n" << "\n";
    return true;
  }
}

bool mini_max::integrity::Checker::TestLayer(unsigned int layer_number) {
  /* database must be open */
  if (!db.IsOpen()) {
    log << "ERROR: Database file not open!" << "\n";
    return ReturnValues::FalseOrStop();
  }

  /* do not read state by state from file, but load the full layer into memory
   */
  /* once */
  if (max_num_states_to_test > load_full_layer_threshold) {
    db.SetLoadingOfFullLayerOnRead();
  }

  /* run test */
  return StartTestThreads(layer_number, TestLayerThreadProc);
}

DWORD mini_max::integrity::Checker::TestLayerThreadProc(void* p_parameter,
                                                        int64_t index) {
  /* locals */
  CheckerThreadVars& tl_vars = *((CheckerThreadVars*)p_parameter);
  Checker& c = tl_vars.r_checker;
  database::Database& db = c.db;
  GameInterface& game = c.game;
  Logger& log = c.log;
  unsigned int layer_number = tl_vars.layer_number;
  unsigned int state_number = index;
  unsigned int thread_no = tl_vars.cur_thread_no;
  auto& sub_value_in_database = tl_vars.sub_value_in_database;
  auto& sub_ply_infos = tl_vars.sub_ply_infos;
  std::vector<bool>& has_cur_player_changed = tl_vars.has_cur_player_changed;
  std::vector<unsigned int>& possibility_ids = tl_vars.possibility_ids;
  TwoBit short_value_in_database;
  TwoBit short_value_in_game;
  PlyInfoVarType num_plies_till_cur_state;
  PlyInfoVarType min, max;
  float float_value_in_game;
  unsigned int num_possibilities;
  unsigned int i, j;
  unsigned int tmp_state_number, tmp_layer_number;
  unsigned int sym_op;
  void* p_backup;
  bool invalid_layer_or_state_number;
  bool layer_in_database_and_completed;
  bool player_has_changed;

  /* output */
  tl_vars.states_processed.StateProcessed(
      log, db.GetNumberOfKnots(layer_number), /* text: */ L"Tested ");

  /* situation already existend in database ? */
  db.ReadKnotValueFromDatabase(layer_number, state_number,
                               short_value_in_database);
  db.ReadPlyInfoFromDatabase(layer_number, state_number,
                             num_plies_till_cur_state);

  /* prepare the situation */
  if (!game.SetSituation(thread_no, /* layer_num: */ layer_number,
                         state_number)) {
    /* when situation cannot be constructed then state must be marked as invalid
     */
    /* in database */
    if (short_value_in_database != SKV_VALUE_INVALID ||
        num_plies_till_cur_state != PLYINFO_VALUE_INVALID) {
      log << "ERROR: DATABASE ERROR IN LAYER " << layer_number << " AND STATE "
          << state_number
          << ": Could not set situation, but value is not invalid." << "\n";
      return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
    } else {
      return TM_RETURN_VALUE_OK;
    }
  }

  /* debug information */
  if (c.verbosity > 5) {
    log.Log(Logger::LogLevel::trace,
            /* message: */ L"Test layer: " + std::to_wstring(layer_number) +
                L" state: " + std::to_wstring(state_number));
    game.PrintField(thread_no, short_value_in_database);
  }

  /* get number of possiblities */
  game.GetPossibilities(thread_no, possibility_ids);
  num_possibilities = (unsigned int)possibility_ids.size();

  /* unable to move */
  if (num_possibilities == 0) {
    /* get ingame value */
    game.GetValueOfSituation(thread_no, float_value_in_game,
                             short_value_in_game);

    /* compare database with game */
    if (short_value_in_database != short_value_in_game ||
        num_plies_till_cur_state != 0) {
      log << "ERROR: DATABASE ERROR IN LAYER " << layer_number << " AND STATE "
          << state_number
          << ": Number of possibilities is zero, but knot value is not invalid "
             "or ply info equal zero."
          << "\n";
      return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
    }
    if (short_value_in_database == SKV_VALUE_INVALID) {
      log << "ERROR: DATABASE ERROR IN LAYER " << layer_number << " AND STATE "
          << state_number
          << ": Number of possibilities is zero, but knot value is invalid."
          << "\n";
      return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
    }

  } else {
    /* check each possible move */
    for (i = 0; i < num_possibilities; i++) {
      /* move */
      game.Move(thread_no, /* id_possibility: */ possibility_ids[i],
                player_has_changed, p_backup);
      has_cur_player_changed[i] = player_has_changed;

      /* get database value */
      game.GetLayerAndStateNumber(thread_no, tmp_layer_number, tmp_state_number,
                                  sym_op);
      layer_in_database_and_completed =
          db.IsLayerCompleteAndInFile(tmp_layer_number);
      /* db.IsComplete() must be true, since after removing the stone from Layer
       */
      /* 101 State 660201181 we are going to Layer 105, which is not calculated
       */
      /* yet. */
      if (!layer_in_database_and_completed && db.IsComplete()) {
        log << "ERROR: DATABASE ERROR IN LAYER " << layer_number
            << " AND STATE " << state_number << ": State " << tmp_state_number
            << " of layer " << tmp_layer_number
            << " after move is not in database or not completed." << "\n";
        return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
      }
      if (i >= sub_ply_infos.size() || i >= sub_value_in_database.size()) {
        log << "ERROR: GetMaxNumPossibilities() returns a value smaller than "
               "the number of possible moves."
            << "\n";
        return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
      }
      db.ReadKnotValueFromDatabase(tmp_layer_number, tmp_state_number,
                                   sub_value_in_database[i]);
      db.ReadPlyInfoFromDatabase(tmp_layer_number, tmp_state_number,
                                 sub_ply_infos[i]);

      /* debug information */
      if (c.verbosity > 5) {
        log.Log(Logger::LogLevel::trace,
                /* message: */ L"Test layer: " + std::to_wstring(layer_number) +
                    L" state: " + std::to_wstring(state_number));
        game.PrintField(thread_no, sub_value_in_database[i],
                        /* indent_spaces: */ 4);
      }

      /* if layer or state number is invalid then value of testes state must be
       */
      /* invalid */
      if (sub_value_in_database[i] == SKV_VALUE_INVALID &&
          short_value_in_database != SKV_VALUE_INVALID) {
        log << "ERROR: DATABASE ERROR IN LAYER " << layer_number
            << " AND STATE " << state_number
            << ": Succeding state  has invalid layer (" << tmp_layer_number
            << ") or state number (" << tmp_state_number
            << "), but tested state is not marked as invalid." << "\n";
        return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
      }

      /* undo move */
      game.Undo(thread_no, /* id_possibility: */ possibility_ids[i],
                player_has_changed, p_backup);
      has_cur_player_changed[i] = player_has_changed;
    }

    /* value possible? */
    switch (short_value_in_database) {
      case SKV_VALUE_GAME_LOST:

        /* all possible moves must be lost for the current player or won for the
         */
        /* opponent */
        for (i = 0; i < num_possibilities; i++) {
          if (sub_value_in_database[i] != ((has_cur_player_changed[i])
                                               ? SKV_VALUE_GAME_WON
                                               : SKV_VALUE_GAME_LOST) &&
              sub_value_in_database[i] != SKV_VALUE_INVALID) {
            log << "ERROR: DATABASE ERROR IN LAYER " << layer_number
                << " AND STATE " << state_number
                << ": All possible moves must be lost for the current player "
                   "or won for the opponent"
                << "\n";
            return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
          }
        }
        /* not all options can be invalid */
        for (j = 0, i = 0; i < num_possibilities; i++) {
          if (sub_value_in_database[i] == SKV_VALUE_INVALID) {
            j++;
          }
        }
        if (j == num_possibilities) {
          log << "DATABASE ERROR IN LAYER " << layer_number << " AND STATE "
              << state_number << ". Not all options can be invalid" << "\n";
          return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
        }
        /* ply info must be max(sub_ply_infos[]+1) */
        max = 0;
        for (i = 0; i < num_possibilities; i++) {
          if (sub_value_in_database[i] == ((has_cur_player_changed[i])
                                               ? SKV_VALUE_GAME_WON
                                               : SKV_VALUE_GAME_LOST)) {
            if (sub_ply_infos[i] + 1 > max) {
              max = sub_ply_infos[i] + 1;
            }
          }
        }
        if (num_plies_till_cur_state > PLYINFO_VALUE_DRAWN) {
          log << "DATABASE ERROR IN LAYER " << layer_number << " AND STATE "
              << state_number
              << ": Knot value is LOST, but num_plies_till_cur_state is bigger "
                 "than PLYINFO_MAX_VALUE."
              << "\n";
          return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
        }
        if (num_plies_till_cur_state != max) {
          log << "Number of plies:     " << num_plies_till_cur_state << "\n";
          log << "Max number of plies: " << max << "\n";
          game.PrintField(thread_no, short_value_in_database);
          log << "DATABASE ERROR IN LAYER " << layer_number << " AND STATE "
              << state_number
              << ": Number of needed plies is not maximal for LOST state."
              << "\n";
          return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
        }
        break;

      case SKV_VALUE_GAME_WON:

        /* at least one possible move must be lost for the opponent or won for
         */
        /* the current player */
        {
          bool found = false;
          for (i = 0; i < num_possibilities; i++) {
            if (sub_value_in_database[i] == ((has_cur_player_changed[i])
                                                 ? SKV_VALUE_GAME_LOST
                                                 : SKV_VALUE_GAME_WON)) {
              found = true;
              break;
            }
          }
          if (!found) {
            log << "DATABASE ERROR IN LAYER " << layer_number << " AND STATE "
                << state_number
                << ": At least one possible move must be lost for the opponent "
                   "or won for the current player."
                << "\n";
            return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
          }
        }

        /* ply info must be min(sub_ply_infos[]+1) */
        min = PLYINFO_VALUE_DRAWN;
        for (i = 0; i < num_possibilities; i++) {
          if (sub_value_in_database[i] == ((has_cur_player_changed[i])
                                               ? SKV_VALUE_GAME_LOST
                                               : SKV_VALUE_GAME_WON)) {
            if (sub_ply_infos[i] + 1 < min) {
              min = sub_ply_infos[i] + 1;
            }
          }
        }
        if (num_plies_till_cur_state > PLYINFO_VALUE_DRAWN) {
          log << "DATABASE ERROR IN LAYER " << layer_number << " AND STATE "
              << state_number
              << ": Knot value is WON, but num_plies_till_cur_state is bigger "
                 "than "
                 "PLYINFO_MAX_VALUE."
              << "\n";
          return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
        }
        if (num_plies_till_cur_state != min) {
          log << "DATABASE ERROR IN LAYER " << layer_number << " AND STATE "
              << state_number
              << ": Number of needed plies is not minimal for WON state."
              << "\n";
          log << "num_plies_till_cur_state: " << num_plies_till_cur_state
              << "\n";
          log << "min: " << min << "\n";
          log << "sub_ply_infos: ";
          for (i = 0; i < num_possibilities; i++) {
            log << sub_ply_infos[i] << " ";
          }
          log << "\n";
          log << "sub_value_in_database: ";
          for (i = 0; i < num_possibilities; i++) {
            log << sub_value_in_database[i] << " ";
          }
          log << "\n";
          log << "has_cur_player_changed: ";
          for (i = 0; i < num_possibilities; i++) {
            log << has_cur_player_changed[i] << " ";
          }
          log << "\n";
          log << "possibility_ids: ";
          for (i = 0; i < num_possibilities; i++) {
            log << possibility_ids[i] << " ";
          }
          log << "\n";
          return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
        }
        break;

      case SKV_VALUE_GAME_DRAWN:

        /* all possible moves must be won for the opponent, lost for the current
         */
        /* player or drawn */
        for (j = 0, i = 0; i < num_possibilities; i++) {
          if (sub_value_in_database[i] != ((has_cur_player_changed[i])
                                               ? SKV_VALUE_GAME_WON
                                               : SKV_VALUE_GAME_LOST) &&
              sub_value_in_database[i] != SKV_VALUE_GAME_DRAWN &&
              sub_value_in_database[i] != SKV_VALUE_INVALID) {
            log << "DATABASE ERROR IN LAYER " << layer_number << " AND STATE "
                << state_number
                << ": All possible moves must be won for the opponent, lost "
                   "for the current player or drawn."
                << "\n";
            return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
          }
          if (sub_value_in_database[i] == SKV_VALUE_GAME_DRAWN) {
            j = 1;
          }
        }

        /* at least one succeding state must be drawn */
        if (j == 0) {
          log << "DATABASE ERROR IN LAYER " << layer_number << " AND STATE "
              << state_number << ": At least one succeding state must be drawn."
              << "\n";
          return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
        }

        /* ply info must also be drawn */
        if (num_plies_till_cur_state != PLYINFO_VALUE_DRAWN) {
          log << "DATABASE ERROR IN LAYER " << layer_number << " AND STATE "
              << state_number << ": Knot value is drawn but ply info is not!"
              << "\n";
          return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
        }
        break;

      case SKV_VALUE_INVALID:
        /* if SetSituation() returned true but state value is invalid, then all
         */
        /* following states must be invalid */
        for (i = 0; i < num_possibilities; i++) {
          if (sub_value_in_database[i] != SKV_VALUE_INVALID) {
            break;
          }
        }
        if (i != num_possibilities) {
          log << "DATABASE ERROR IN LAYER " << layer_number << " AND STATE "
              << state_number
              << ": If SetSituation() returned true but state value is "
                 "invalid, then all following states must be invalid."
              << "\n";
          return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
        }
        /* ply info must also be invalid */
        if (num_plies_till_cur_state != PLYINFO_VALUE_INVALID) {
          log << "DATABASE ERROR IN LAYER " << layer_number << " AND STATE "
              << state_number << ": Knot value is invalid but ply info is not!"
              << "\n";
          return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
        }
        break;
    }
  }
  return TM_RETURN_VALUE_OK;
}

/* Like TestLayer(), but tests only a single state in the database */
bool mini_max::integrity::Checker::TestState(unsigned int layer_number,
                                             unsigned int state_number) {
  CheckerThreadVars tl_vars(*this, layer_number, max_num_branches,
                            rough_total_num_states_processed);
  return TestLayerThreadProc(&tl_vars, state_number) == TM_RETURN_VALUE_OK;
}

bool mini_max::integrity::Checker::TestIfSymStatesHaveSameValue(
    unsigned int layer_number) {
  /* test if each state has symmetric states with the same value */
  log << "\n"
      << "*** Test if symmetric states have same value in layer: "
      << layer_number << "***\n";
  log << game.GetOutputInformation(layer_number) << "\n";

  /* simple checks */
  if (!db.IsOpen()) {
    return log.Log(Logger::LogLevel::error,
                   /* message: */ L"ERROR: No database file open!");
  }
  if (!db.IsLayerCompleteAndInFile(layer_number)) {
    return log.Log(Logger::LogLevel::error,
                   /* message: */ L"ERROR: Layer not in file!");
  }

  /* do not read state by state from file, but load the full layer into memory
   */
  /* once */
  if (max_num_states_to_test > load_full_layer_threshold) {
    db.SetLoadingOfFullLayerOnRead();
  }

  /* run test */
  return StartTestThreads(layer_number, TestSymStatesSameValueThreadProc);
}

DWORD mini_max::integrity::Checker::TestSymStatesSameValueThreadProc(
    void* p_parameter, int64_t index) {
  /* Locals */
  CheckerThreadVars& tl_vars = *((CheckerThreadVars*)p_parameter);
  Checker& c = tl_vars.r_checker;
  database::Database& db = c.db;
  GameInterface& game = c.game;
  Logger& log = c.log;
  unsigned int layer_number = tl_vars.layer_number;
  unsigned int state_number = index;
  unsigned int thread_no = tl_vars.cur_thread_no;
  TwoBit short_value_in_database;
  TwoBit short_value_of_sym_state;
  PlyInfoVarType num_plies_till_cur_state;
  PlyInfoVarType num_plies_till_sym_state;
  std::vector<StateAddressStruct> sym_states;

  /* output */
  tl_vars.states_processed.StateProcessed(
      log, db.GetNumberOfKnots(layer_number), /* text: */ L"Tested ");

  /* situation already existend in database ? */
  db.ReadKnotValueFromDatabase(layer_number, state_number,
                               short_value_in_database);
  db.ReadPlyInfoFromDatabase(layer_number, state_number,
                             num_plies_till_cur_state);

  /* prepare the situation */
  if (!game.SetSituation(thread_no, /* layer_num: */ layer_number,
                         state_number)) {
    /* when situation cannot be constructed then state must be marked as invalid
     */
    /* in database */
    if (short_value_in_database != SKV_VALUE_INVALID ||
        num_plies_till_cur_state != PLYINFO_VALUE_INVALID) {
      log << "ERROR: DATABASE ERROR IN LAYER " << layer_number << " AND STATE "
          << state_number
          << ": Could not set situation, but value is not invalid." << "\n";
      return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
    } else {
      return TM_RETURN_VALUE_OK;
    }
  }

  /* get numbers of symmetric states */
  game.GetSymStateNumWithDuplicates(thread_no, sym_states);

  /* save value for all symmetric states */
  for (auto& sym_state : sym_states) {
    db.ReadKnotValueFromDatabase(sym_state.layer_number, sym_state.state_number,
                                 short_value_of_sym_state);
    db.ReadPlyInfoFromDatabase(sym_state.layer_number, sym_state.state_number,
                               num_plies_till_sym_state);

    /* values of symmetric states must be equal to the value of the current */
    /* state */
    if (short_value_of_sym_state != short_value_in_database ||
        num_plies_till_cur_state != num_plies_till_sym_state) {
      log.Log(Logger::LogLevel::error,
              /* message: */ L"current tested state " +
                  std::to_wstring(state_number) + L" has value " +
                  std::to_wstring((int)short_value_in_database));
      game.SetSituation(thread_no, /* layer_num: */ layer_number, state_number);
      game.PrintField(thread_no, short_value_in_database);
      log << "\n";
      log << "symmetric layer " << sym_state.layer_number << " and state "
          << sym_state.state_number << " has value "
          << (int)short_value_of_sym_state << "\n";
      game.SetSituation(thread_no, /* layer_num: */ sym_state.layer_number,
                        sym_state.state_number);
      game.PrintField(thread_no, short_value_of_sym_state);
      game.SetSituation(thread_no, /* layer_num: */ layer_number, state_number);
      return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
    }
  }
  return TM_RETURN_VALUE_OK;
}

bool mini_max::integrity::Checker::TestSetSituationAndGetStateNum(
    unsigned int layer_number) {
  log << "\n"
      << "*** Test SetSituation() and GetLayerAndStateNumber() ***" << "\n";
  return StartTestThreads(layer_number, TestSetSituationThreadProc);
}

bool mini_max::integrity::Checker::TestMoveAndUndo(unsigned int layer_number) {
  log << "\n" << "*** Test Move() and Undo() ***" << "\n";
  /* get succeeding layers (here game.GetSuccLayers() is avoided due to */
  /* performance reason, db.GetSuccLayers() is cached) */
  game.GetSuccLayers(layer_number, succ_layers);
  return StartTestThreads(layer_number, TestMoveAndUndoThreadProc);
}

bool mini_max::integrity::Checker::TestGetPredecessors(
    unsigned int layer_number) {
  log << "\n" << "*** Test GetPredecessors() ***" << "\n";
  return StartTestThreads(layer_number, TestGetPredecessorsThreadProc);
}

bool mini_max::integrity::Checker::TestGetPossibilities(
    unsigned int layer_number) {
  log << "\n" << "*** Test GetPossibilities() ***" << "\n";
  return StartTestThreads(layer_number, TestGetPossibilitiesThreadProc);
}

DWORD mini_max::integrity::Checker::TestSetSituationThreadProc(
    void* p_parameter, int64_t index) {
  /* locals */
  CheckerThreadVars& tl_vars = *((CheckerThreadVars*)p_parameter);
  Checker& c = tl_vars.r_checker;
  GameInterface& game = c.game;
  Logger& log = c.log;
  StateAddressStruct cur_state = {/* state_number: */ (StateNumberVarType)index,
                                  (unsigned char)tl_vars.layer_number};

  /* game.GetLayerAndStateNumber() */
  StateAddressStruct got_state;
  unsigned int got_sym_op, got_layer_number;

  /* game.GetSymStateNumWithDuplicates() */
  std::vector<StateAddressStruct> sym_states;

  /* game.GetValueOfSituation() */
  float float_value;
  TwoBit short_value;

  /* set state */
  if (game.SetSituation(tl_vars.cur_thread_no,
                        /* layer_num: */ cur_state.layer_number,
                        cur_state.state_number)) {
    /* get symmetry operation number */
    game.GetLayerAndStateNumber(tl_vars.cur_thread_no, got_layer_number,
                                got_state.state_number, got_sym_op);
    got_state.layer_number = (unsigned char)got_layer_number;

    /* get value of situation */
    game.GetValueOfSituation(tl_vars.cur_thread_no, float_value, short_value);

    /* check if state is any duplicate */
    game.GetSymStateNumWithDuplicates(tl_vars.cur_thread_no, sym_states);

    if (std::find(/* first: */ sym_states.begin(), /* last: */ sym_states.end(),
                  got_state) == sym_states.end()) {
      log << "ERROR: SetSituation(" << cur_state.layer_number << ", "
          << cur_state.state_number << "), but GetLayerAndStateNumber("
          << got_state.layer_number << ", " << got_state.state_number
          << ") is not listed in GetSymStateNumWithDuplicates()!" << "\n";
      return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
    }

    /* compare states from SetSituation() and GetLayerAndStateNumber(), they */
    /* must be equal */
    if (cur_state.layer_number != got_state.layer_number ||
        cur_state.state_number != got_state.state_number) {
      log << "ERROR: SetSituation(" << cur_state.layer_number << ", "
          << cur_state.state_number << "), but GetLayerAndStateNumber("
          << got_state.layer_number << ", " << got_state.state_number << ")!"
          << "\n";
      return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
    }

    /* value of situation must not be invalid now, otherwise SetSituation() */
    /* should have returned false */
    if (short_value == SKV_VALUE_INVALID) {
      log << "ERROR: SetSituation(" << cur_state.layer_number << ", "
          << cur_state.state_number
          << ") == true, but GetValueOfSituation() == SKV_VALUE_INVALID!"
          << "\n";
      return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
    }
  }

  /* output */
  tl_vars.states_processed.StateProcessed(
      log, c.db.GetNumberOfKnots(cur_state.layer_number),
      /* text: */ L"Tested ");
  return TM_RETURN_VALUE_OK;
}

DWORD mini_max::integrity::Checker::TestMoveAndUndoThreadProc(void* p_parameter,
                                                              int64_t index) {
  /* locals */
  CheckerThreadVars& tl_vars = *((CheckerThreadVars*)p_parameter);
  Checker& c = tl_vars.r_checker;
  GameInterface& game = c.game;
  Logger& log = c.log;
  StateAddressStruct cur_state = {/* state_number: */ (StateNumberVarType)index,
                                  (unsigned char)tl_vars.layer_number};

  /* game.GetPossibilities() */
  std::vector<unsigned int> possibility_ids;
  void* p_backup = nullptr;
  unsigned int cur_poss = 0;
  alpha_beta::KnotStruct knot;
  alpha_beta::KnotStruct sub_knot;

  /* game.GetValueOfSituation() */
  float float_value = 0;
  TwoBit short_knot_value = SKV_VALUE_GAME_DRAWN;

  /* game.GetLayerAndStateNumber() */
  StateAddressStruct sub_state;

  /* set state */
  if (game.SetSituation(tl_vars.cur_thread_no,
                        /* layer_num: */ cur_state.layer_number,
                        cur_state.state_number)) {
    game.GetValueOfSituation(tl_vars.cur_thread_no, float_value,
                             short_knot_value);
  } else {
    short_knot_value = SKV_VALUE_INVALID;
  }

  if (c.verbosity >= 5) {
    game.PrintField(tl_vars.cur_thread_no, /* value: */ 0,
                    /* indent_spaces: */ 0);
  }

  /* is current state consistent? */
  if (short_knot_value != SKV_VALUE_INVALID &&
      !game.IsStateIntegrityOk(tl_vars.cur_thread_no)) {
    log << "ERROR: SetSituation(" << cur_state.layer_number << ", "
        << cur_state.state_number
        << ") returned true, but fieldIntegrity() is NOT ok!" << "\n";
    return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
  }

  /* get number of possiblities */
  game.GetPossibilities(tl_vars.cur_thread_no, possibility_ids);
  knot.num_possibilities = (unsigned int)possibility_ids.size();

  /* unable to move */
  if (knot.num_possibilities == 0) {
    if (short_knot_value == SKV_VALUE_GAME_DRAWN &&
        game.LostIfUnableToMove(tl_vars.cur_thread_no)) {
      log << "ERROR: SetSituation(" << cur_state.layer_number << ", "
          << cur_state.state_number
          << ") returned true and GetValueOfSituation() returned DRAWN, "
             "although GetPossibilities() yields no possible moves."
          << "\n";
      return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
    }
    /* moving is possible */
  } else {
    if (short_knot_value == SKV_VALUE_INVALID) {
      log << "ERROR: SetSituation(" << cur_state.layer_number << ", "
          << cur_state.state_number
          << ")==false or GetValueOfSituation()==SKV_VALUE_INVALID, now "
             "GetPossibilities() yields some possible moves."
          << "\n";
      return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
    }

    /* check each possibility */
    for (cur_poss = 0; cur_poss < knot.num_possibilities; cur_poss++) {
      /* move */
      game.Move(tl_vars.cur_thread_no,
                /* id_possibility: */ possibility_ids[cur_poss],
                sub_knot.player_to_move_changed, p_backup);

      if (c.verbosity >= 5) {
        game.PrintField(tl_vars.cur_thread_no, /* value: */ 0,
                        /* indent_spaces: */ 0);
      }

      /* is this layer listed in GetSuccLayers() */
      if (std::find(/* first: */ c.succ_layers.begin(),
                    /* last: */ c.succ_layers.end(),
                    sub_state.layer_number) == c.succ_layers.end()) {
        /* BUG: Fix TicTacToe and Muehle */
        /* log << "ERROR: SetSituation(" << cur_state.layer_number << ", " << */
        /* cur_state.state_number << ") -> Move() -> GetLayerAndStateNumber(" <<
         */
        /* sub_state.layer_number << ", " << sub_state.state_number << "). */
        /* Succeding state not listed in GameInterface::GetSuccLayers()!" << */
        /* "\n"; return TM_RETURN_VALUE_TERMINATE_ALL_THREADS; */
      }

      /* get state number of succeding state */
      unsigned int sub_state_layer, sym_op;
      game.GetLayerAndStateNumber(tl_vars.cur_thread_no, sub_state_layer,
                                  sub_state.state_number, sym_op);
      sub_state.layer_number = sub_state_layer;

      /* is current consistent? */
      if (!game.IsStateIntegrityOk(tl_vars.cur_thread_no)) {
        log << "ERROR: SetSituation(" << cur_state.layer_number << ", "
            << cur_state.state_number
            << ") -> Move() -> GetLayerAndStateNumber("
            << sub_state.layer_number << ", " << sub_state.state_number
            << "). Now fieldIntegrity() is NOT ok!" << "\n";
        return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
      }

      /* undo move */
      game.Undo(tl_vars.cur_thread_no,
                /* id_possibility: */ possibility_ids[cur_poss],
                knot.player_to_move_changed, p_backup);

      /* did player_to_move_changed correctly? */
      if (knot.player_to_move_changed != sub_knot.player_to_move_changed) {
        log << "ERROR: Move(player_to_move_changed="
            << sub_knot.player_to_move_changed
            << ") -> Undo(player_to_move_changed="
            << knot.player_to_move_changed << ")!" << "\n";
        return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
      }

      /* did we came back to original state? */
      unsigned int got_layer_number, got_state_number, got_sym_op;
      game.GetLayerAndStateNumber(tl_vars.cur_thread_no, got_layer_number,
                                  got_state_number, got_sym_op);
      if (cur_state.layer_number != got_layer_number ||
          cur_state.state_number != got_state_number) {
        log << "ERROR: SetSituation(" << cur_state.layer_number << ", "
            << cur_state.state_number << ") -> Move(" << cur_poss
            << ") -> Undo(" << cur_poss << ") -> GetLayerAndStateNumber("
            << got_layer_number << ", " << got_state_number << ")!" << "\n";
        return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
      }

      /* state reached by Move() must not be invalid */
      if (!game.SetSituation(tl_vars.cur_thread_no,
                             /* layer_num: */ sub_state.layer_number,
                             sub_state.state_number)) {
        log << "ERROR: Moved from layer " << cur_state.layer_number
            << " and state " << cur_state.state_number
            << " to an invalid situation layer " << sub_state.layer_number
            << " and state " << sub_state.state_number << "\n";
        return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
      }

      /* set back to current state */
      game.SetSituation(tl_vars.cur_thread_no,
                        /* layer_num: */ cur_state.layer_number,
                        cur_state.state_number);
    }
  }

  /* output */
  tl_vars.states_processed.StateProcessed(
      log, c.db.GetNumberOfKnots(cur_state.layer_number),
      /* text: */ L"Tested ");
  return TM_RETURN_VALUE_OK;
}

DWORD mini_max::integrity::Checker::TestGetPredecessorsThreadProc(
    void* p_parameter, int64_t index) {
  /* locals */
  CheckerThreadVars& tl_vars = *((CheckerThreadVars*)p_parameter);
  Checker& c = tl_vars.r_checker;
  GameInterface& game = c.game;
  Logger& log = c.log;
  StateAddressStruct cur_state = {/* state_number: */ (StateNumberVarType)index,
                                  (unsigned char)tl_vars.layer_number};

  /* game.GetPossibilities() */
  std::vector<unsigned int> possibility_ids;
  void* p_backup = nullptr;
  unsigned int cur_poss = 0;
  alpha_beta::KnotStruct knot;

  /* game.GetPredecessors() */
  std::vector<retro_analysis::PredVars> pred_vars;
  pred_vars.reserve(/* n: */ MAX_NUM_PREDECESSORS);

  /* print status */
  tl_vars.states_processed.StateProcessed(
      log, c.db.GetNumberOfKnots(cur_state.layer_number),
      /* text: */ L"Tested ");

  /* set situation of a valid state */
  if (!game.SetSituation(tl_vars.cur_thread_no,
                         /* layer_num: */ cur_state.layer_number,
                         cur_state.state_number)) {
    return TM_RETURN_VALUE_OK;
  }

  if (c.verbosity >= 5) {
    log.Log(Logger::LogLevel::trace,
            /* message: */ L"SetSituation(" +
                std::to_wstring(cur_state.layer_number) + L", " +
                std::to_wstring(cur_state.state_number) + L")");
    game.PrintField(tl_vars.cur_thread_no, /* value: */ 0,
                    /* indent_spaces: */ 0);
  }

  /* get all possible moves */
  game.GetPossibilities(tl_vars.cur_thread_no, possibility_ids);
  knot.num_possibilities = (unsigned int)possibility_ids.size();

  /* go to each successor state */
  for (cur_poss = 0; cur_poss < knot.num_possibilities; cur_poss++) {
    /* move */
    game.Move(tl_vars.cur_thread_no,
              /* id_possibility: */ possibility_ids[cur_poss],
              knot.player_to_move_changed, p_backup);

    if (c.verbosity >= 5) {
      log.Log(Logger::LogLevel::trace,
              /* message: */ L"Move() according possibility_id=" +
                  std::to_wstring(possibility_ids[cur_poss]));
      game.PrintField(tl_vars.cur_thread_no, /* value: */ 0,
                      /* indent_spaces: */ 4);
    }

    /* get predecessors */
    game.GetPredecessors(tl_vars.cur_thread_no, pred_vars);

    if (c.verbosity >= 5) {
      log << "GetPredecessors()" << "\n";
      for (auto& cur_pred_var : pred_vars) {
        log << "          layer_number=" << cur_pred_var.pred_layer_number
            << ", state_number=" << cur_pred_var.pred_state_number
            << ", sym_operation =" << cur_pred_var.pred_sym_operation
            << ", player_to_move_changed="
            << cur_pred_var.player_to_move_changed << "\n";
      }
    }

    /* is original state listed in pred_vars? */
    unsigned int j;
    for (j = 0; j < pred_vars.size(); j++) {
      if (pred_vars[j].pred_state_number == cur_state.state_number &&
          pred_vars[j].pred_layer_number == cur_state.layer_number) {
        break;
      }
    }
    if (j == pred_vars.size()) {
      log << "ERROR: Layer " << cur_state.layer_number << " and state "
          << cur_state.state_number << " not found in predecessor list!"
          << "\n";
      return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
    }

    /* is player_to_move_changed consistend between Move() and GetPredecessors()
     */
    if (pred_vars[j].player_to_move_changed != knot.player_to_move_changed) {
      log << "ERROR: SetSituation(" << cur_state.layer_number << " and state "
          << cur_state.state_number
          << ") -> Move(player_to_move_changed=" << knot.player_to_move_changed
          << "), but GetPredecessors(player_to_move_changed="
          << pred_vars[j].player_to_move_changed << ")!" << "\n";
      return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
    }

    /* undo move */
    game.Undo(tl_vars.cur_thread_no,
              /* id_possibility: */ possibility_ids[cur_poss],
              knot.player_to_move_changed, p_backup);

    if (c.verbosity >= 5) {
      log << "Undo()" << "\n";
      game.PrintField(tl_vars.cur_thread_no, /* value: */ 0,
                      /* indent_spaces: */ 4);
    }
  }

  /* output */
  return TM_RETURN_VALUE_OK;
}

DWORD mini_max::integrity::Checker::TestGetPossibilitiesThreadProc(
    void* p_parameter, int64_t index) {
  /* locals */
  CheckerThreadVars& tl_vars = *((CheckerThreadVars*)p_parameter);
  Checker& c = tl_vars.r_checker;
  GameInterface& game = c.game;
  Logger& log = c.log;
  StateAddressStruct cur_state = {/* state_number: */ (StateNumberVarType)index,
                                  (unsigned char)tl_vars.layer_number};

  /* game.GetPossibilities() */
  std::vector<unsigned int> possibility_ids;
  void* p_backup = nullptr;
  unsigned int cur_poss = 0;
  alpha_beta::KnotStruct knot;

  /* game.GetValueOfSituation() */
  float float_value = 0;
  TwoBit short_knot_value = SKV_VALUE_GAME_DRAWN;

  /* game.GetPredecessors() */
  std::vector<retro_analysis::PredVars> pred_vars;
  pred_vars.reserve(/* n: */ MAX_NUM_PREDECESSORS);

  /* game.GetLayerAndStateNumber() */
  StateAddressStruct got_state;
  unsigned int got_sym_op, got_layer_number;

  /* print status */
  tl_vars.states_processed.StateProcessed(
      log, c.db.GetNumberOfKnots(cur_state.layer_number),
      /* text: */ L"Tested ");

  /* set situation of a valid state */
  if (!game.SetSituation(tl_vars.cur_thread_no,
                         /* layer_num: */ cur_state.layer_number,
                         cur_state.state_number)) {
    return TM_RETURN_VALUE_OK;
  }

  if (c.verbosity >= 5) {
    log.Log(Logger::LogLevel::trace,
            /* messages: */ L"SetSituation(" +
                std::to_wstring(cur_state.layer_number) + L", " +
                std::to_wstring(cur_state.state_number) + L")");
    game.PrintField(tl_vars.cur_thread_no, /* value: */ 0,
                    /* indent_spaces: */ 0);
  }

  /* get predecessors */
  game.GetPredecessors(tl_vars.cur_thread_no, pred_vars);

  /* test each returned predecessor */
  unsigned int j;
  for (j = 0; j < pred_vars.size(); j++) {
    /* set situation */
    if (!game.SetSituation(tl_vars.cur_thread_no,
                           pred_vars[j].pred_layer_number,
                           pred_vars[j].pred_state_number)) {
      log << "ERROR: Could not SetSituation(" << pred_vars[j].pred_layer_number
          << ", " << pred_vars[j].pred_state_number
          << "), which was returned by GetPredecessors("
          << cur_state.layer_number << ", " << cur_state.state_number << ")."
          << "\n";
      return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
    }

    /* Apply the symmetry operation used to reach this predecessor state,
       ensuring the game situation matches the predecessor's representation. */
    game.ApplySymOp(tl_vars.cur_thread_no, pred_vars[j].pred_sym_operation,
                    /* do_inverse_operation: */ true,
                    /* player_to_move_changed: */ false);

    if (c.verbosity >= 5) {
      log.Log(Logger::LogLevel::trace, /* message: */ L"predecessor state:");
      game.PrintField(tl_vars.cur_thread_no, /* value: */ 0,
                      /* indent_spaces: */ 0);
    }

    /* get all possible moves */
    game.GetPossibilities(tl_vars.cur_thread_no, possibility_ids);
    knot.num_possibilities = (unsigned int)possibility_ids.size();
    if (!knot.num_possibilities) {
      log << "ERROR: GetPredecessors(" << cur_state.layer_number << ", "
          << cur_state.state_number << ") -> SetSituation("
          << pred_vars[j].pred_layer_number << ", "
          << pred_vars[j].pred_state_number << ") -> ApplySymOp("
          << pred_vars[j].pred_sym_operation
          << ") -> GetPossibilities() yields no possible moves." << "\n";
      return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
    }

    /* go to each successor state */
    for (cur_poss = 0; cur_poss < knot.num_possibilities; cur_poss++) {
      /* move */
      game.Move(tl_vars.cur_thread_no,
                /* id_possibility: */ possibility_ids[cur_poss],
                knot.player_to_move_changed, p_backup);

      /* get corresponding state number */
      game.GetLayerAndStateNumber(tl_vars.cur_thread_no, got_layer_number,
                                  got_state.state_number, got_sym_op);
      got_state.layer_number = (unsigned char)got_layer_number;

      /* does states match ? */
      if (cur_state.layer_number == got_state.layer_number &&
          cur_state.state_number == got_state.state_number) {
        break;
      }

      /* undo move */
      game.Undo(tl_vars.cur_thread_no,
                /* id_possibility: */ possibility_ids[cur_poss],
                knot.player_to_move_changed, p_backup);
    }
    if (cur_poss == knot.num_possibilities) {
      log << "ERROR: Not all predecessors states lead back to the original "
             "layer "
          << cur_state.layer_number << " and state " << cur_state.state_number
          << " by calling Move()." << "\n";
      return TM_RETURN_VALUE_TERMINATE_ALL_THREADS;
    }
  }

  /* output */
  return TM_RETURN_VALUE_OK;
}

} /* namespace muehle */