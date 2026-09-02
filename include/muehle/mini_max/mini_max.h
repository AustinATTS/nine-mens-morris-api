#ifndef MUEHLE_MINI_MAX_MINI_MAX_H_
#define MUEHLE_MINI_MAX_MINI_MAX_H_

#include <numeric>

#include "muehle/mini_max/alpha_beta/solver.h"
#include "muehle/mini_max/integrity/checker_thread_vars.h"
#include "muehle/mini_max/state_info.h"
#include "muehle/mini_max/statistics/monitor.h"

namespace muehle {

/* Player: Either ego or opponent. The player to move is the current player
 * Layer: The states are divided into layers. For example depending on the
 * number of stones on the field. State: A unique game state representing a
 * current game situation. Knot: Each knot of the graph corresponds to a game
 * state. The knots are connected by possible valid moves. Ply Info: Number of
 * plies/moves necessary to win the game. State Adress: A state is identified by
 * the corresponding layer and the state number within the layer. Short Knot
 * Value: Each knot/state can have the value SKV_VALUE_INVALID,
 * SKV_VALUE_GAME_LOST, SKV_VALUE_GAME_DRAWN, or SKV_VALUE_GAME_WON. Float Point
 * Knot Value: Each knot/state can be evaluated by a floating point value. High
 * positive values represents winning situations. Negative values stand for
 * losing situations. Database: The database contains the arrays with the short
 * knot values and the ply infos.
 */

namespace mini_max {
/* The MinMax class manages the MinMax algorithm and related operations for game
 * state evaluation.
 * Reponsobilities:
 * - Interfaces with the game logic to perform minimax and alpha-beta pruning
 * searches
 * - Manages the database of evaluated game states and their values.
 * - Coordinates multi-threaded calculations and progress reporting.
 * - Provides functions for querying best moves, statistics, and inregrity
 * checks.
 * Usage:
 * - Instantiate with a GameInterface pointer and desired search depth.
 * - Use public methods to open/calculate databases, get best choices, and
 * manage threading. */
class MiniMax {
  friend class MinMaxWinInspectDb;
  friend class MinMaxWinCalcDb;
  friend class MinMaxDbCompTrans;
  friend class statistics::Monitor;

 private:
  GameInterface* game = nullptr;
  Logger log;

 public:
  /* Constructor / destructor */
  MiniMax(GameInterface* game, unsigned int max_alpha_beta_search_depth);
  ~MiniMax();

  database::Database db;
  integrity::Checker checker;
  statistics::Monitor monitor;

  /* Getter */
  unsigned int GetNumThreads();
  bool AnyFeshlyCalculatedLayer();

  /* Functions for getting the best choice */
  bool GetBestChoice(unsigned int& choice, StateInfo& info_about_choices);
  void SetSearchDepth(unsigned int max_alpha_beta_search_depth);

  /* Database functions */
  bool OpenDatabase(std::wstring const& directory,
                    bool use_comp_file_if_both_exist = true);
  bool CalculateDatabase();
  bool CalculateStatistics();
  bool IsCurrentStateInDatabase(unsigned int thread_no);
  void UnloadDatabase();
  void CloseDatabase();
  void PauseDatabaseCalculation();
  void CancelDatabaseCalculation();
  bool WasDatabaseCalculationCancelled();
  unsigned int GetLastCalculatedLayer();
  bool SetOutputStream(std::wostream& the_stream);
  bool SetNumThreads(unsigned int num_threads);

 private:
  /* Variables that typically remain unchanged during database calculation */
  std::wstring file_directory; /* Path of the folder where the database files
                                  are located */
  std::list<unsigned int>
      last_calculated_layer; /* List of the recentyl calculated layers */
  std::vector<unsigned int>
      layers_to_calculate; /* Layers to calculate, in case multiple layers must
                              be calculated at once */
  ThreadManagerClass thread_manager; /* Thread manager for multi threading */
  CRITICAL_SECTION
  cs_os_print; /* For thread safety when output is passed to os_print */

  /* Solvers */
  alpha_beta::Solver ab_solver;
  retro_analysis::Solver rt_solver;

  /* Thread specific or non constant variables */
  long long num_states_processed =
      0; /* Number of states processed by all threads */
  unsigned int cur_calculated_layer =
      0; /* Id of the currently calculated layer */

  /* Progress report functions */
  bool CalcLayer(unsigned int layer_number);
  void SetCurrentActivity(Activity new_action);
};

}  // namespace mini_max
}  // namespace muehle

#endif  // MUEHLE_MINI_MAX_MINI_MAX_H_
