#include "muehle/mini_max/retro_analysis/successor_count_array.h"

#include "muehle/mini_max/database/array_info_struct.h"

namespace muehle {

/* Constructor for the successor count array */
mini_max::retro_analysis::SuccessorCountArray::SuccessorCountArray(
    Logger& log, database::Database& db, unsigned int layer_number)
    : log(log), db(db), layer_number(layer_number) {
  /* Allocate memory for count arrays and set default value to 0 */
  long long num_knots_in_cur_layer = db.GetNumberOfKnots(layer_number);
  succ_count_array.resize(num_knots_in_cur_layer, 0);
  db.array_infos.AddArray(
      layer_number, database::ArrayInfoStruct::ArrayType::count_array,
      num_knots_in_cur_layer * sizeof(CountArrayVarType), 0);
}

/* Delete successor count array and remove it from the database array
 * information */
mini_max::retro_analysis::SuccessorCountArray::~SuccessorCountArray() {
  if (!db.GetNumberOfKnots(layer_number)) {
    return;
  }
  db.array_infos.RemoveArray(
      layer_number, database::ArrayInfoStruct::ArrayType::count_array,
      db.GetNumberOfKnots(layer_number) * sizeof(CountArrayVarType), 0);
}

/* Increase the successor counter of the state by one in a NON thread safe way.
 * The counter is increased by one, if the state is a possible move for the
 * current state.
 * Returns the new counter value, after increasing. */
mini_max::CountArrayVarType
mini_max::retro_analysis::SuccessorCountArray::IncreaseCounter(
    StateNumberVarType state_number) {
  if (state_number >= succ_count_array.size()) {
    log.Log(Logger::LogLevel::error, L"State number is out of range!");
    return COUNT_ARRAY_MAX_VALUE;
  }

  CountArrayVarType& count_value = succ_count_array[state_number];
  if (count_value == COUNT_ARRAY_MAX_VALUE) {
    log.Log(Logger::LogLevel::error, L"Maxmimum value for Count[] reached!");
    return COUNT_ARRAY_MAX_VALUE;
  } else {
    count_value++;
  }
  return count_value;
}

/* Decrease the successor counter of the state by one in a thread safe way.
 * If the counter is already zero, the function will return
 * COUNT_ARRAY_MAX_VALUE.
 * Returns the new counter value, after decreasing. */
mini_max::CountArrayVarType
mini_max::retro_analysis::SuccessorCountArray::DecreaseCounter(
    StateNumberVarType state_number) {
  if (state_number >= succ_count_array.size()) {
    log.Log(Logger::LogLevel::error, L"State number is out of range!");
    return COUNT_ARRAY_MAX_VALUE;
  }

  std::lock_guard<std::mutex> lock(succ_count_array_mutex);
  CountArrayVarType& count_value = succ_count_array[state_number];
  if (count_value > 0) {
    count_value--;
  } else {
    log.Log(Logger::LogLevel::error, L"Count is already zero!");
    return COUNT_ARRAY_MAX_VALUE;
  }
  return count_value;
}

}  // namespace muehle
