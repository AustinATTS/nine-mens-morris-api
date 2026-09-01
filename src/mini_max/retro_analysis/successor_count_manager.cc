#include "muehle/mini_max/retro_analysis/successor_count_manager.h"

namespace muehle {

// Implementation goes here.

mini_max::retro_analysis::SuccessorCountManager::SuccessorCountManager(
    Logger& log, ThreadManagerClass& tm, database::Database& db,
    GameInterface& game, std::vector<StateQueue>& states_to_process) {
}

mini_max::retro_analysis::SuccessorCountManager::~SuccessorCountManager() {
}

bool mini_max::retro_analysis::SuccessorCountManager::Init(
    std::vector<unsigned int>& layers_to_calculate) {
}

bool mini_max::retro_analysis::SuccessorCountManager::IsReady() {
}

mini_max::CountArrayVarType mini_max::retro_analysis::SuccessorCountManager::
GetAndDecreaseCounter(unsigned int layer_number,
    StateNumberVarType state_number) {
}

bool mini_max::retro_analysis::SuccessorCountManager::InitLayer(
    SuccessorCountArray& sca) {
}

bool mini_max::retro_analysis::SuccessorCountManager::CalcNumSuccedors(
    unsigned int layer_number) {
}

bool mini_max::retro_analysis::SuccessorCountManager::AddNumSuccedors(
    unsigned int layer_number) {
}

DWORD mini_max::retro_analysis::SuccessorCountManager::
AddNumSuccedorsThreadProc(void* p_parameter, int64_t index) {
}
}  // namespace muehle
