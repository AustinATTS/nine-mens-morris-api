#ifndef MUEHLE_MINI_MAX_RETURN_VALUES_H_
#define MUEHLE_MINI_MAX_RETURN_VALUES_H_

namespace muehle {
namespace mini_max {

class ReturnValues {
 public:
  static const bool stop_con_critical_error =
      false; /* If true, the program will stop on critical erros */

  static bool FalseOrStop();
};

}  // namespace mini_max
}  // namespace muehle

#endif  // MUEHLE_MINI_MAX_RETURN_VALUES_H_
