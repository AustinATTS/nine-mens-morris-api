#include "muehle/mini_max/return_values.h"

namespace muehle {
namespace mini_max {

/* If stop_on_critical_error is set to true, the program will stop.
 * This is practical for debugging. */
bool mini_max::ReturnValues::FalseOrStop() {
  if (ReturnValues::stop_con_critical_error) {
    WaitForSingleObject(/* h: */ GetCurrentProcess(), INFINITE);
  }
  return false;
}

} /* namespace mini_max */
} /* namespace muehle */
