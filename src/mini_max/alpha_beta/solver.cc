#include "muehle/mini_max/alpha_beta/solver.h"

namespace muehle {

// Implementation goes here.

mini_max::alpha_beta::Solver::Solver(Logger& log, ThreadManagerClass& tm,
    database::Database& db, GameInterface& game) {
}

bool mini_max::alpha_beta::Solver::GetBestChoice(unsigned int& choice,
    StateInfo& info_about_choices) {
}

bool mini_max::alpha_beta::Solver::CalcKnotValueByAlphaBeta(
    std::vector<unsigned int>& layers_to_calculate) {
}

void mini_max::alpha_beta::Solver::SetSearchDepth(
    unsigned int max_alpha_beta_search_depth) {
}

bool mini_max::alpha_beta::Solver::Init(unsigned int layer_number) {
}

bool mini_max::alpha_beta::Solver::Run(unsigned int layer_number) {
}

bool mini_max::alpha_beta::Solver::LetTheTreeGrow(KnotStruct& knot,
    RunAlphaBetaVars& rab_vars, unsigned int til_level, float alpha,
    float beta) {
}

bool mini_max::alpha_beta::Solver::TryDatabase(KnotStruct& knot,
    const RunAlphaBetaVars& rab_vars, unsigned int til_level,
    unsigned int& layer_number, unsigned int& state_number) {
}

bool mini_max::alpha_beta::Solver::TryPossibilities(KnotStruct& knot,
    RunAlphaBetaVars& rab_vars, unsigned int til_level,
    unsigned int& max_won_freq_values_sub_moves, float& alpha, float& beta) {
}

bool mini_max::alpha_beta::Solver::SaveInDatabase(const KnotStruct& knot,
    RunAlphaBetaVars& rab_vars, unsigned int layer_number,
    unsigned int state_number) {
}

DWORD mini_max::alpha_beta::Solver::InitThreadProc(void* p_parameter,
    int64_t index) {
}

DWORD mini_max::alpha_beta::Solver::RunThreadProc(void* p_parameter,
    int64_t index) {
}

DWORD mini_max::alpha_beta::Solver::MinMaxThreadProc(void* p_parameter,
    int64_t index) {
}
}  // namespace muehle
