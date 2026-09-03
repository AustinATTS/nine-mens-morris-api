#ifndef MUEHLE_MINI_MAX_RETRO_ANALYSIS_SUCCESSOR_COUNT_FILE_STORAGE_H_
#define MUEHLE_MINI_MAX_RETRO_ANALYSIS_SUCCESSOR_COUNT_FILE_STORAGE_H_

#include "muehle/mini_max/database/database.h"
#include "muehle/mini_max/retro_analysis/state_queue.h"
#include "muehle/mini_max/retro_analysis/successor_count_array.h"
#include "muehle/mini_max/return_values.h"
#include "muehle/mini_max/type_def.h"
#include "muehle/utils/logger.h"
#include "muehle/utils/thread_manager_class.h"

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
    /* Layer number */
    unsigned int layer_number;

    /* Number of knots in the current layer */
    unsigned int num_knots_in_layer;

    /* Successor count array for the current layer */
    SuccessorCountArray* succ_count_array = nullptr;

    /* User needs to provide only the layer number and the number of knots in
     * the current layer */
    LayerInfoStruct() {};
    LayerInfoStruct(unsigned int layer_number, unsigned int num_knots_in_layer,
                    SuccessorCountArray& succ_count_array)
        : layer_number(layer_number),
          num_knots_in_layer(num_knots_in_layer),
          succ_count_array(&succ_count_array) {}

    /* File handle for loading and saving the arrays in 'count_arrays' */
    HANDLE h_file_count_array = NULL;

    /* File path for the count array file */
    std::wstring s_count_array_file_path;
  };

  SuccessorCountFileStorage(Logger& log, const std::wstring& file_directory,
                            std::vector<LayerInfoStruct>& layers_to_calculate);
  ~SuccessorCountFileStorage();
  bool Write();
  bool Read();

 private:
  /* Logger, used for output */
  Logger& log;

  /* Layer number */
  std::vector<LayerInfoStruct> layers_to_calculate;
};

} /* namespace retro_analysis */
} /* namespace mini_max */
} /* namespace muehle */

#endif /* MUEHLE_MINI_MAX_RETRO_ANALYSIS_SUCCESSOR_COUNT_FILE_STORAGE_H_ */
