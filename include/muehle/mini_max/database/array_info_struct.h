#ifndef MUEHLE_MINI_MAX_DATABASE_ARRAY_INFO_STRUCT_H_
#define MUEHLE_MINI_MAX_DATABASE_ARRAY_INFO_STRUCT_H_

#include <string>

namespace muehle {
namespace mini_max {
namespace database {

/* Structure holding the information about one big array used in the database */
struct ArrayInfoStruct {
  enum class ArrayType : unsigned int {
    invalid = 0,
    knot_already_calculated,
    count_array,
    ply_infos,
    layer_stats,
    size
  };

  /* Use case of the array */
  ArrayType type = ArrayType::invalid;

  /* Size of the array in bytes */
  long long size_in_bytes = 0;

  /* Size of the array in bytes after compression */
  long long compresed_size_in_bytes = 0;

  /* Layer number the array belongs to */
  unsigned int belongs_to_layer = 0;

  const std::wstring GetArrTypeName();
};

} /* namespace database */
} /* namespace mini_max */
} /* namespace muehle */

#endif /* MUEHLE_MINI_MAX_DATABASE_ARRAY_INFO_STRUCT_H_ */