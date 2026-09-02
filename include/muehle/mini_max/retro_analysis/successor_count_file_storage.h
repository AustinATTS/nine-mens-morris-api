#ifndef MUEHLE_MINI_MAX_RETRO_ANALYSIS_SUCCESSOR_COUNT_FILE_STORAGE_H_
#define MUEHLE_MINI_MAX_RETRO_ANALYSIS_SUCCESSOR_COUNT_FILE_STORAGE_H_

#include "muehle/mini_max/retro_analysis/successor_count_array.h"
#include "muehle/mini_max/return_values.h"
#include "muehle/mini_max/database/database.h"
#include "muehle/utils/logger.h"
#include "muehle/utils/thread_manager_class.h"
#include "muehle/mini_max/type_def.h"
#include "muehle/mini_max/retro_analysis/state_queue.h"

namespace muehle {
namespace mini_max {
namespace retro_analysis {

/* Class for storing the succ_count_arrays in files
 * - The successor count arrays are stored in a file, one for each layer
 * - All layers to calculate must be loaded/stored at once
 * - The file is opened in the constructor and closed in the destructor
 * - The file is opened in binary mode and the data is written in binary format
 */
class SuccessorCountFileStorage {
 public:
  struct LayerInfoStruct {
    unsigned int layer_number;       /* Layer number */
    unsigned int num_knots_in_layer; /* Number of knots in the current layer */
    SuccessorCountArray* succ_count_array =
        nullptr; /* Successor count array for the current layer */

    /* User needs to provide only the layer number and the number of knots in
     * the current layer */
    LayerInfoStruct() {};
    LayerInfoStruct(unsigned int layer_number, unsigned int num_knots_in_layer,
                    SuccessorCountArray& succ_count_array)
        : layer_number(layer_number),
          num_knots_in_layer(num_knots_in_layer),
          succ_count_array(&succ_count_array) {}

    /* Reserved for internal use */
    HANDLE h_file_count_array = NULL; /* File handle for loading and saving the
                                         arrays in 'count_arrays' */
    std::wstring
        s_count_array_file_path; /* File path for the count array file */
  };

  SuccessorCountFileStorage(Logger& log, const std::wstring& file_directory,
                            std::vector<LayerInfoStruct>& layers_to_calculate);
  ~SuccessorCountFileStorage();
  bool Write();
  bool Read();

 private:
  Logger& log; /* Logger, used for output */
  std::vector<LayerInfoStruct> layers_to_calculate; /* Layer number */
};

}  // namespace retro_analysis
}  // namespace mini_max
}  // namespace muehle

#endif  // MUEHLE_MINI_MAX_RETRO_ANALYSIS_SUCCESSOR_COUNT_FILE_STORAGE_H_
