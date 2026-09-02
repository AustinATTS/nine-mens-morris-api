#ifndef MUEHLE_MINI_MAX_INTEGRITY_CHECKER_H_
#define MUEHLE_MINI_MAX_INTEGRITY_CHECKER_H_

#include "muehle/mini_max/database/database.h"
#include "muehle/utils/logger.h"
#include "muehle/utils/thread_manager_class.h"
#include "muehle/mini_max/type_def.h"
#include "muehle/utils/my_string.h"

namespace muehle {
namespace mini_max {
namespace integrity {

/* Class for testing the integrity of the Database and the GameInterface */
class Checker {
  friend class CheckerThreadVars;

public:
  Checker(Logger& log, ThreadManagerClass& tm, database::Database& db, GameInterface& game);

  /* Tests GameInterface */
  bool TestSetSituationAndGetStateNum(unsigned int layer_number);
  bool TestMoveAndUndo(unsigned int layer_number);
  bool TestGetPredecessors(unsigned int layer_number);
  bool TestGetPossibilities(unsigned int layer_number);

  /* Tests Database */
  bool TestState(unsigned int layer_number, unsigned int state_number);
  bool TestLayer(unsigned int layer_number);
  bool TestIfSymStatesHaveSameValue(unsigned int layer_number);

  /* Settings */
  void SetOutputFrequency(unsigned int output_every_nth_states) {
    this->output_every_nth_states = output_every_nth_states;
  }
  void SetMaxNumStatesToTest(unsigned int max_num_states_to_test) {
    this->max_num_states_to_test = max_num_states_to_test;
  }

private:
  /* Variables */
  GameInterface& game;
  Logger& log;
  ThreadManagerClass& tm;
  database::Database& db;
  int64_t rough_total_num_states_processed = 0;
  int64_t num_states_processed = 0;
  unsigned int verbosity = 2;
#ifdef _DEBUG
  unsigned int output_every_nth_states = 10000;
  unsigned int load_full_layer_threshold = 10000;
#else // _DEBUG
  unsigned int output_every_nth_states = 10000000;
  unsigned int load_full_layer_threshold = 100000;
#endif // _DEBUG
  unsigned int max_num_branches = 0;
  unsigned int max_num_states_to_test = std::numeric_limits<unsigned int>::max();
  std::vector<unsigned int> succ_layers; /* Stores the indices of successor layers for integrity checks */

  /* Helper functions */
  bool StartTestThreads(unsigned int layer_number, DWORD thread_prc(void* p_parameter, int64_t index));
  unsigned int GetIncrement(unsigned int layer_number);

  /* Static thread functions */
  static DWORD TestLayerThreadProc(void* p_parameter, int64_t index);
  static DWORD TestMoveAndUndoThreadProc(void* p_parameter, int64_t index);
  static DWORD TestSetSituationThreadProc(void* p_parameter, int64_t index);
  static DWORD TestGetPredecessorsThreadProc(void* p_parameter, int64_t index);
  static DWORD TestGetPossibilitiesThreadProc(void* p_parameter, int64_t index);
  static DWORD TestSymStatesSameValueThreadProc(void* p_parameter, int64_t index);
};

} // namespace integrity
} // namespace mini_max
}  // namespace muehle

#endif  // MUEHLE_MINI_MAX_INTEGRITY_CHECKER_H_
