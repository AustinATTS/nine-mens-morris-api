#ifndef MUEHLE_MINI_MAX_RETRO_ANALYSIS_STATE_QUEUE_H_
#define MUEHLE_MINI_MAX_RETRO_ANALYSIS_STATE_QUEUE_H_

#include "muehle/mini_max/state_address_struct.h"
#include "muehle/mini_max/type_def.h"
#include "muehle/utils/cyclic_array.h"
#include "muehle/utils/logger.h"

namespace muehle {
namespace mini_max {
namespace retro_analysis {

/* Class containing a queue with a large amount of states to process.
 * There is one queue for each ply number, managed as a cyclic array.
 * Each cyclic array is stored in a file, allowing for efficient disk-backed
 * store and retreival. The cyclic arrays are dynamically resized as needed, and
 * each queue maintains its own state count. Usage Pattern: Each thread should
 * instantiate its own StateQueue instance to avoid concurrency issues. The
 * class is not thread safe. Concurrent access must be managed externally.
 * Expected usage: Push states to the queue for processing, pop states when
 * processed, and resize queues as the search depth changes. */
class StateQueue {
 public:
  StateQueue(Logger& log, const std::wstring& file_directory,
             unsigned int thread_no);
  ~StateQueue();

  StateQueue(StateQueue&& other) noexcept;
  StateQueue& operator=(StateQueue&& other) noexcept;

  bool Resize(PlyInfoVarType ply_number, size_t number_of_knots);
  bool PushBack(const StateAddressStruct& state, PlyInfoVarType ply_number,
                StateNumberVarType number_of_knots);
  bool PopFront(StateAddressStruct& state, PlyInfoVarType ply_number);
  unsigned int Size(PlyInfoVarType ply_number);
  long long GetNumStatesToProcess() {
    return num_states_to_process;
  }
  PlyInfoVarType GetMaxPlyInfoValue() {
    return max_ply_info_value;
  }

 private:
  /* Logger, used for output */
  Logger& log;

  /* Cyclic array containing the states, whose short know values are known for
   * sure. They have to be processed */
  std::vector<CyclicArray*> states_to_process;

  /* Number of states in 'states_to_process' which have to be processed */
  long long num_states_to_process = 0;

  /* Maximum ply info value */
  PlyInfoVarType max_ply_info_value = 0;

  /* Directory where the files are stored */
  std::wstring file_directory;

  /* Thread number, used for file names */
  unsigned int thread_no = 0xffff;

  /* Align to cache line (64 bytes) */
  alignas(64) char dummy_cache_align;
};

} /* namespace retro_analysis */
} /* namespace mini_max */
} /* namespace muehle */

#endif /* MUEHLE_MINI_MAX_RETRO_ANALYSIS_STATE_QUEUE_H_ */
