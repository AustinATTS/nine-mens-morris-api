#include "muehle/mini_max/state_address_struct.h"

namespace muehle {
namespace mini_max {

/* Compares the state number and layer number */
bool mini_max::StateAddressStruct::operator==(
    const StateAddressStruct& rhs) const {
  return (state_number == rhs.state_number && layer_number == rhs.layer_number);
}

/* Compares the state number and layer number */
bool mini_max::StateAddressStruct::operator<(
    const StateAddressStruct& rhs) const {
  if (layer_number < rhs.layer_number) {
    return true;
  }
  if (layer_number > rhs.layer_number) {
    return false;
  }
  return (state_number < rhs.state_number);
}

/* Compares the state number and layer number */
bool mini_max::StateAddressStruct::operator>(
    const StateAddressStruct& rhs) const {
  if (layer_number > rhs.layer_number) {
    return true;
  }
  if (layer_number < rhs.layer_number) {
    return false;
  }
  return (state_number > rhs.state_number);
}

}  // namespace mini_max
}  // namespace muehle
