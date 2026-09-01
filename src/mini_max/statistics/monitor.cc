#include "muehle/mini_max/statistics/monitor.h"

namespace muehle {

// Implementation goes here.

void mini_max::statistics::Monitor::ShowMemoryStatus() {
}

bool mini_max::statistics::Monitor::CalcLayerStatistics(
    const char* statistics_file_name) {
}

bool mini_max::statistics::Monitor::CalcMaxPlyInfo(unsigned int layer_number,
    PlyInfoVarType& max_ply_info_won,
    StateNumberVarType& max_ply_info_state_won,
    PlyInfoVarType& max_ply_info_lost, StateNumberVarType& max_ply_state_lost) {
}

void mini_max::statistics::Monitor::GetCurrentCalculatedLayer(
    std::vector<unsigned int>& layers) {
}

const wchar_t* mini_max::statistics::Monitor::GetCurrentActionStr() {
}

mini_max::statistics::Monitor::Monitor(MiniMax* m, Logger& log) {
}
}  // namespace muehle
