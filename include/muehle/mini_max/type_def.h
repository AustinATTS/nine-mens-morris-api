#ifndef MUEHLE_MINI_MAX_TYPE_DEF_H_
#define MUEHLE_MINI_MAX_TYPE_DEF_H_

#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else /* _WIN32 */
#include "muehle/win_32_compat.h"
#endif /* _WIN32 */

#include "muehle/utils/logger.h"

namespace muehle {
namespace mini_max {

/* Macros */
#define SAFE_DELETE(p) \
  {                    \
    if (p) {           \
      delete (p);      \
      (p) = NULL;      \
    }                  \
  }
#define SAFE_DELETE_ARRAY(p) \
  {                          \
    if (p) {                 \
      delete[] (p);          \
      (p) = NULL;            \
    }                        \
  }

/* Typedefines */

/* 2-Bit variable ranging from 0 to 3 */
typedef unsigned char TwoBit;

/* 2 Bytes for saving the ply info */
typedef unsigned short PlyInfoVarType;

/* 4 Bytes for addressing states within a layer. */
typedef unsigned int StateNumberVarType;

/* Constants */

/* Minimum float point knot value */
const float FPKV_INV_VALUE = -100001.0f;

/* Minimum float point knot value */
const float FPKV_MIN_VALUE = -100000.0f;

/* Maximum float point knot value */
const float FPKV_MAX_VALUE = 100000.0f;

/* Threshold used when choosing best move. Know values differing less than this
 * threshold will be regarded as egal */
const float FPKV_THRESHOLD = 0.001f;

/* Short knot value: knot value is valid */
const size_t SKV_VALUE_INVALID = 0;

/* Game lost means that there is no perfect move possible */
const size_t SKV_VALUE_GAME_LOST = 1;

/* The perfect move leads to at least a drawn game */
const size_t SKV_VALUE_GAME_DRAWN = 2;

/* The perfect move will lead to a won game */
const size_t SKV_VALUE_GAME_WON = 3;

/* Number of different short know values */
const size_t SKV_NUM_VALUES = 4;

/* Four short knot values are stored in one byte. So all four knot values are
 * invalid */
const size_t SKV_WHOLE_BYTE_IS_INVALID = 0;

/* Expected maximum number of plies -> used as a size for vector initialisation
 */
const size_t PLYINFO_EXP_VALUE = 1000;

/* Knot value is drawn. Since drawn means a never ending game, this is a special
 * ply info */
const PlyInfoVarType PLYINFO_VALUE_DRAWN = 65001;

/* Ply info is not calculated yet for this game state */
const PlyInfoVarType PLYINFO_VALUE_UNCALCULATED = 65002;

/* Ply info is invalid, since knot value is invalid */
const PlyInfoVarType PLYINFO_VALUE_INVALID = 65003;

#ifdef _DEBUG
/* Prints progress every n-th processed knot */
const long long OUTPUT_EVERY_N_STATES = 1000000;
#else  /* _DEBUG */
/* Prints progress every n-th processed knot */
const long long OUTPUT_EVERY_N_STATES = 10000000;
#endif /* _DEBUG */

/* BLOCK_SIZE_IN_CYCLIC_ARRAY*sizeof(StateAdressStruct) = block size in bytes
 * for the cyclic arrays */
const size_t BLOCK_SIZE_IN_CYCLIC_ARRAY = 10000;

/* Maximum number of predecessors. Important for array sizes */
const size_t MAX_NUM_PREDECESSORS = 10000;

/* Size in bytes */
const size_t FILE_BUFFER_SIZE = 1000000;

/* Player to move changed - First index of the 2D-array
 * skv_perspective_matrix[][] */
const size_t PL_TO_MOVE_CHANGED = 0;

/* Player to move is still the same - Second index of the 2D-array
 * skv_perspective_matrix[][] */
const size_t PL_TO_MOVE_UNCHANGED = 1;

/* For io operations per second: measure time every n-th operations */
const long long MEASURE_TIME_FREQUENCY = 100000;

/* True or false - for measurement of the input/output operations per second */
const bool MEASURE_IOPS = false;

/* [Short knot value][PL_TO_MOVE_UNCHANGED/PL_TO_MOVE_CHANGED] - A winning
 * situtation is a losing situation for the oppoent and so on... */
const unsigned char skv_perspective_matrix[SKV_NUM_VALUES][2] = {
    /* PL_TO_MOVE_CHANGED | PL_TO_MOVE_UNCHANGED */
    {
        SKV_VALUE_INVALID,
        SKV_VALUE_INVALID,
    }, /* SKV_VALUE_INVALID */
    {
        SKV_VALUE_GAME_WON,
        SKV_VALUE_GAME_LOST,
    }, /* SKV_VALUE_GAME_LOST */
    {
        SKV_VALUE_GAME_DRAWN,
        SKV_VALUE_GAME_DRAWN,
    }, /* SKV_VALUE_GAME_DRAWN */
    {
        SKV_VALUE_GAME_LOST,
        SKV_VALUE_GAME_WON,
    } /* SKV_VALUE_GAME_WON */
};

/* Map for float values of short values */
const float skv_float_value_map[SKV_NUM_VALUES] = {
    FPKV_MIN_VALUE - 1.0f, /* SKV_VALUE_INVALID */
    FPKV_MIN_VALUE,        /* SKV_VALUE_GAME_LOST */
    0,                     /* SKV_VALUE_GAME_DRAWN */
    FPKV_MAX_VALUE         /* SKV_VALUE_GAME_WON */
};

/* Enums */
enum class Activity {
  init_retro_analysis,
  prepare_count_array,
  perform_retro_analysis,
  perform_alpha_beta,
  testing_layer,
  saving_layer_to_file,
  loading_layer_from_file,
  calc_layer_stats,
  none
};

/* Globally defined current action */
static Activity cur_action = Activity::none;

/* Pre declaration */
class MiniMax;
class GameInterface;

namespace database {
class Database;
} /* namespace database */

namespace retro_analysis {
class Solver;
} /* namespace retro_analysis */

namespace alpha_beta {
class Solver;
} /* namespace alpha_beta */

namespace statistics {
class Monitor;
} /* namespace statistics */

namespace test {
class Tester;
} /* namespace test */

} /* namespace mini_max */
} /* namespace muehle */

#endif /* MUEHLE_MINI_MAX_TYPE_DEF_H_ */
