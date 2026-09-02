#ifndef MUEHLE_MINI_MAX_ALPHA_BETA_COMMON_THREAD_VARS_H_
#define MUEHLE_MINI_MAX_ALPHA_BETA_COMMON_THREAD_VARS_H_

#ifdef _MSC_VER
#define NOMINMAX /* Prevents macro conflicts with min/max in Windows headers \
                  */
#endif           // _MSC_VER
#include "muehle/mini_max/progress_counter.h"
#include "muehle/mini_max/type_def.h"
#include "muehle/utils/logger.h"
#include "muehle/utils/thread_manager_class.h"

namespace muehle {
namespace mini_max {

/* Base class for thread specific variables. It provides ReadByte() and
 * WriteByte() for buffered file access and a mutex for synchronisation of the
 * buffer access. Although writing is thread safe, no overlapping writing is
 * allowed. Since the file is opened in exclusive move, only one thread can
 * write to the file at a time. */
class CommonThreadVars : public ThreadManagerClass::ThreadVarsArrayItem {
 private:
  char padding[64]; /* Padding to avoid cache coherance issues */
  const unsigned int max_buffer_size =
      FILE_BUFFER_SIZE; /* Maximum size of the buffer in bytes */
  HANDLE h_file = INVALID_HANDLE_VALUE; /* Handle of the file */
  std::wstring file_path; /* File path for the file. Same for all threads */
  int64_t file_size = 0;  /* Size in bytes of the file at the moment */
  int64_t file_position =
      0; /* Position within the file, being the state number */
  int64_t target_file_size = 0; /* Target file size in bytes, being the number
                                   of states in the layer */
  int64_t buffer_position = 0;  /* Position within the buffer */
  int64_t buffer_offset = 0;    /* offset for the buffer within the file */
  int64_t& total_num_states_processed; /* Total number of states processed by
                                          all threads */

  static std::mutex
      buffer_mutex; /* Mutex for the buffer, since it is used by all threads */
  std::vector<unsigned char>
      buffer; /* Since writing/reading happens byte by byte, a buffer is used to
                 store the data logger, used for output */
  Logger& log;

  bool LoadDataToBuffer();
  bool Flush();

 public:
  unsigned int layer_number = 0;    /* Current calculated layer */
  ProgressCounter states_processed; /* Number of states already calculated in
                                       the current layer */
  bool load_from_file =
      false; /* Flag indicating if the initialisation has already been done */

  CommonThreadVars(CommonThreadVars const& master);
  CommonThreadVars(unsigned int layer_number, const std::wstring& file_path,
                   int64_t target_file_size,
                   int64_t& rough_total_num_states_processed,
                   int64_t& total_num_states_processed, Logger& log);
  ~CommonThreadVars();

  bool ReadByte(int64_t position_in_file, unsigned char& data);
  bool WriteByte(int64_t position_in_file, unsigned char data);
  void Reduce() override;
};

}  // namespace mini_max
}  // namespace muehle

#endif  // MUEHLE_MINI_MAX_ALPHA_BETA_COMMON_THREAD_VARS_H_
