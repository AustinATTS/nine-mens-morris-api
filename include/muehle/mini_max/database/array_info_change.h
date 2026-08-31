#ifndef MUEHLE_MINI_MAX_DATABASE_ARRAY_INFO_CHANGE_H_
#define MUEHLE_MINI_MAX_DATABASE_ARRAY_INFO_CHANGE_H_

#include "muehle/mini_max/database/array_info_struct.h"

namespace muehle {
namespace mini_max {
namespace database {

/* Helper structure to update the GUI */
struct ArrayInfoChange {
  unsigned int item_index = 0xffffffff;  /* Index of the GUI element */
  ArrayInfoStruct* array_info = nullptr; /* Pointer to the array info, which has
                                            to be updated. Can be a nullptr */
};

}  // namespace database
}  // namespace mini_max
}  // namespace muehle

#endif  // MUEHLE_MINI_MAX_DATABASE_ARRAY_INFO_CHANGE_H_
