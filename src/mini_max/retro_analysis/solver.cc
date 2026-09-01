#include "muehle/mini_max/retro_analysis/solver.h"

namespace muehle {

// Implementation goes here.

mini_max::retro_analysis::Solver::Solver(Logger& log, ThreadManagerClass& tm,
    database::Database& db, GameInterface game) {
}

mini_max::retro_analysis::Solver::~Solver() {
}

bool mini_max::retro_analysis::Solver::CalcKnotValuesByRetroAnalysis(
    std::vector<unsigned int>& layers_to_calculate) {
}

bool mini_max::retro_analysis::Solver::InitRetroAnalysis() {
}

bool mini_max::retro_analysis::Solver::PrepareCountArrays() {
}

bool mini_max::retro_analysis::Solver::PerformRetroAnalysis() {
}

bool mini_max::retro_analysis::Solver::ProcessPredecessor(StateQueue& queue,
    const StateAddressStruct& cur_state, const PredVars& pred_vars_state) {
}

size_t mini_max::retro_analysis::Solver::EstimateTotalNumberOfKnots() {
}

DWORD mini_max::retro_analysis::Solver::InitRetroAnalysisThreadProc(
    void* p_parameter, int64_t index) {
}

DWORD mini_max::retro_analysis::Solver::PerformRetroAnalysisThreadProc(
    void* p_parameter) {
}
}  // namespace muehle
