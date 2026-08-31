#ifndef MUEHLE_MINI_MAX_STATE_ADDRESS_STRUCT_H_
#define MUEHLE_MINI_MAX_STATE_ADDRESS_STRUCT_H_

#include "muehle/mini_max/type_def.h"

namespace muehle {
namespace mini_max {

/* Address of a state/knot within the database, representing a unique state. In
 * the real game there might be identical ones due to symmetry. */
struct StateAddressStruct {
  StateNumberVarType state_number; /* State id within the corresponding layer */
  unsigned char layer_number;      /* Layer id */

  bool operator==(const StateAddressStruct& rhs) const;
  bool operator<(const StateAddressStruct& rhs) const;
  bool operator>(const StateAddressStruct& rhs) const;
};

}  // namespace mini_max
}  // namespace muehle

#endif  // MUEHLE_MINI_MAX_STATE_ADDRESS_STRUCT_H_
