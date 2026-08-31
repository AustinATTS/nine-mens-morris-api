#ifndef MUEHLE_MINI_MAX_DATABASE_SPEEDOMETER_H_
#define MUEHLE_MINI_MAX_DATABASE_SPEEDOMETER_H_

#include <functional>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else  // _WIN32
#include "muehle/win_32_compat.h"
#endif  // _WIN32

namespace muehle {
namespace mini_max {
namespace database {

/* Class to measure database io operations per second */
class Speedometer {
 public:
  typedef std::function<void(std::wstring& name, float operations_per_sec)>
      PrintFunctType;

 private:
  long long num_operations =
      0; /* Number of operations done since last interval */
  long long print_every_nth_operations = 0; /* Print every n-th operation */
  LARGE_INTEGER last_time; /* Time of interval for read operations */
  LARGE_INTEGER
      frequency;     /* Performance counter frequency, in counts per second */
  std::wstring name; /* Name of the speedometer */
  PrintFunctType
      print_function; /* Callback function to print the operations per second */
  std::mutex mutex;   /* Mutex for thread safety */

 public:
  Speedometer(std::wstring const& name, long long print_every_nth_operations,
              PrintFunctType print_function);
  ~Speedometer();
  void MeasureIops();
};

}  // namespace database
}  // namespace mini_max
}  // namespace muehle

#endif  // MUEHLE_MINI_MAX_DATABASE_SPEEDOMETER_H_
