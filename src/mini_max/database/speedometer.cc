#include "muehle/mini_max/database/speedometer.h"

#ifdef _WIN32
/* QueryPerformanceCounter, QueryPerformanceFrequency */
#include <windows.h>
#else /* _WIN32 */
#include "muehle/win_32_compat.h"
#endif /* _WIN32 */

namespace muehle {

/* constructor */
mini_max::database::Speedometer::Speedometer(
    std::wstring const& name, long long print_every_nth_operations,
    PrintFunctType print_function)
    : name(name),
      print_every_nth_operations(print_every_nth_operations),
      num_operations(0),
      print_function(print_function) {
  /* for io operations per second measurement */
  QueryPerformanceFrequency(&frequency);
}

/* destructor */
mini_max::database::Speedometer::~Speedometer() {}

void mini_max::database::Speedometer::MeasureIops() {
  /* thread safety */
  std::lock_guard<std::mutex> lock(mutex);

  /* first call */
  if (num_operations == 0) {
    QueryPerformanceCounter(&last_time);
  }

  /* count operation */
  num_operations++;

  if (num_operations >= print_every_nth_operations) {
    LARGE_INTEGER cur_time;
    QueryPerformanceCounter(&cur_time);
    double total_time_gone =
        (double)(cur_time.QuadPart - last_time.QuadPart) / frequency.QuadPart;
    print_function(name, num_operations / total_time_gone);
    last_time.QuadPart = cur_time.QuadPart;
    num_operations = 0;
  }
}

} /* namespace muehle */