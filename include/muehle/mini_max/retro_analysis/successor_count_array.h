#ifndef MUEHLE_MINI_MAX_RETRO_ANALYSIS_SUCCESSOR_COUNT_ARRAY_H_
#define MUEHLE_MINI_MAX_RETRO_ANALYSIS_SUCCESSOR_COUNT_ARRAY_H_

#include "muehle/mini_max/database/database.h"
#include "muehle/mini_max/retro_analysis/state_queue.h"
#include "muehle/mini_max/type_def.h"
#include "muehle/utils/logger.h"
#include "muehle/utils/thread_manager_class.h"

namespace muehle {
namespace mini_max {

/* 2 Bytes for counting predecessors */
typedef unsigned short CountArrayVarType;

/* Maximum value for the count array */
const CountArrayVarType COUNT_ARRAY_MAX_VALUE = 65535;

namespace retro_analysis {

/* Class for the successor count array
 * - decreasing and increasing the counter is thread safe
 * - The number of succeeding states is specific to one layer and is stored in a
 * count array called 'succ_count_array'*/
class SuccessorCountArray {
 public:
  SuccessorCountArray(Logger& log, database::Database& db,
                      unsigned int layer_number);
  ~SuccessorCountArray();
  CountArrayVarType IncreaseCounter(StateNumberVarType state_number);
  CountArrayVarType DecreaseCounter(StateNumberVarType state_number);
  unsigned int GetLayerNumber() const {
    return layer_number;
  }

  /* Logger, used for output */
  Logger& log;

  /* Database, for storing the calculated values */
  database::Database& db;

  /* Layer number */
  const unsigned int layer_number;

  /* Count array for the number of drawn/unknown successors for each state */
  std::vector<CountArrayVarType> succ_count_array;

  /* Mutex for the count array */
  std::mutex succ_count_array_mutex;
};

} /* namespace retro_analysis */
} /* namespace mini_max */
} /* namespace muehle */

#endif /* MUEHLE_MINI_MAX_RETRO_ANALYSIS_SUCCESSOR_COUNT_ARRAY_H_ */
