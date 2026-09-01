#ifndef MUEHLE_MINI_MAX_RETRO_ANALYSIS_SUCCESSOR_COUNT_ARRAY_H_
#define MUEHLE_MINI_MAX_RETRO_ANALYSIS_SUCCESSOR_COUNT_ARRAY_H_

#include "muehle/mini_max/database/database.h"
#include "muehle/utils/logger.h"

namespace muehle {
namespace mini_max {

/* Using 2 Bytes for counting the number of predecessors allows a maximum of
 * 65535 predecessors. */
typedef unsigned short
    CountArrayVarType; /* 2 Bytes for counting predecessors */
const CountArrayVarType COUNT_ARRAY_MAX_VALUE =
    65535; /* Maximum value for the count array */

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
  unsigned int GetLayerNumber() const { return layer_number; }

  Logger& log;            /* Logger, used for output */
  database::Database& db; /* Database, for storing the calculated values */
  const unsigned int layer_number; /* Layer number */
  std::vector<CountArrayVarType>
      succ_count_array; /* Count array for the number of drawn/unknown
                           successors for each state */
  std::mutex succ_count_array_mutex; /* Mutex for the count array */
};

}  // namespace retro_analysis
}  // namespace mini_max
}  // namespace muehle

#endif  // MUEHLE_MINI_MAX_RETRO_ANALYSIS_SUCCESSOR_COUNT_ARRAY_H_
