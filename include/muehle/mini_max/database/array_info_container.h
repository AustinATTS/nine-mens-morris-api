#ifndef MUEHLE_MINI_MAX_DATABASE_ARRAY_INFO_CONTAINER_H_
#define MUEHLE_MINI_MAX_DATABASE_ARRAY_INFO_CONTAINER_H_
#include <list>

#include "muehle/mini_max/database/array_info_change.h"
#include "muehle/mini_max/database/array_info_struct.h"
#include "muehle/utils/logger.h"

namespace muehle {
namespace mini_max {
namespace database {

/* Class to store the memory usage about all the arrays in memory. It also calls
 * a callback function to update the GUI. The class is thread safe and print
 * statements are passed to the logger */
class ArrayInfoContainer {
 public:
  typedef std::list<ArrayInfoStruct>::iterator
      ais_itr; /* Iterator for the list of array info objects */

 private:
  long long memory_used = 0;   /* Total memory in bytes used for storing: ply
                                  information, short knot value and ... */
  unsigned int num_layers = 0; /* Number of layers */
  Logger& log;                 /* Callback function to update the GUI */
  std::list<ArrayInfoChange>
      array_infos_to_be_updated; /* Arrays which have been updated in the GUI */
  std::list<ArrayInfoStruct>
      list_arrays; /* All arrays added via AddArray() in a list */
  std::vector<ais_itr>
      vector_arrays; /* Iterators referencing all arrays. Indexing via
                        [layer_number*ArrayInfoStruct::ArrayType::size + type]
                      */
  std::mutex mutex;  /* Mutex for thread safety */

  size_t GetVectorArrayIndex(unsigned int layer_number,
                             ArrayInfoStruct::ArrayType type);

 public:
  ArrayInfoContainer(Logger& log) : log(log) {};
  ~ArrayInfoContainer() {};

  bool AddArray(unsigned int layer_number, ArrayInfoStruct::ArrayType type,
                long long size, long long compressed_size);
  bool RemoveArray(unsigned int layer_number, ArrayInfoStruct::ArrayType type,
                   long long size, long long compressed_size);

  void Init(unsigned int num_layers);

  bool AnyArrayInfoToUpdate();
  ArrayInfoChange GetArrayInfoForUpdate();
  long long GetMemoryUsed() { return memory_used; };
};

}  // namespace database
}  // namespace mini_max
}  // namespace muehle

#endif  // MUEHLE_MINI_MAX_DATABASE_ARRAY_INFO_CONTAINER_H_
