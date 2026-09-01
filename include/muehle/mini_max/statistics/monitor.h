#ifndef MUEHLE_MINI_MAX_STATISTICS_MONITOR_H_
#define MUEHLE_MINI_MAX_STATISTICS_MONITOR_H_

#include "muehle/mini_max/type_def.h"

namespace muehle {
namespace mini_max {
namespace statistics {

class Monitor {
  friend class MiniMax;

 public:
  void ShowMemoryStatus();
  bool CalcLayerStatistics(const char *statistics_file_name);
  bool CalcMaxPlyInfo(unsigned int layer_number,
                      PlyInfoVarType &max_ply_info_won,
                      StateNumberVarType &max_ply_info_state_won,
                      PlyInfoVarType &max_ply_info_lost,
                      StateNumberVarType &max_ply_state_lost);
  void GetCurrentCalculatedLayer(std::vector<unsigned int> &layers);
  const wchar_t *GetCurrentActionStr();

  Monitor(MiniMax *m, Logger &log);

 private:
  MiniMax *mm = nullptr;
  database::Database *db = nullptr;
  GameInterface *game = nullptr;
  Logger &log;
};
}  // namespace statistics
}  // namespace mini_max
}  // namespace muehle

#endif  // MUEHLE_MINI_MAX_STATISTICS_MONITOR_H_
