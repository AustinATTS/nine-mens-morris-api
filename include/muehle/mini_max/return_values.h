#ifndef MUEHLE_MINI_MAX_RETURN_VALUES_H_
#define MUEHLE_MINI_MAX_RETURN_VALUES_H_

#include "muehle/win_32_compat.h"

namespace muehle {
namespace mini_max {

class ReturnValues {
 public:
  /* If true, the program will stop on critical erros */
  static const bool stop_con_critical_error = false;

  static bool FalseOrStop();
};

} /* namespace mini_max */
} /* namespace muehle */

#endif /* MUEHLE_MINI_MAX_RETURN_VALUES_H_ */
