#include "muehle/mini_max/mini_max.h"

namespace muehle {

// Implementation goes here.

mini_max::MiniMax::MiniMax(GameInterface* game,
    unsigned int max_alpha_beta_search_depth) {
}

mini_max::MiniMax::~MiniMax() {
}

unsigned int mini_max::MiniMax::GetNumThreads() {
}

bool mini_max::MiniMax::AnyFeshlyCalculatedLayer() {
}

bool mini_max::MiniMax::GetBestChoice(unsigned int& choice,
    StateInfo& info_about_choices) {
}

void mini_max::MiniMax::
SetSearchDepth(unsigned int max_alpha_beta_search_depth) {
}

bool mini_max::MiniMax::OpenDatabase(std::wstring const& directory,
    bool use_comp_file_if_both_exist) {
}

bool mini_max::MiniMax::CalculateDatabase() {
}

bool mini_max::MiniMax::CalculateStatistics() {
}

bool mini_max::MiniMax::IsCurrentStateInDatabase(unsigned int thread_no) {
}

void mini_max::MiniMax::UnloadDatabase() {
}

void mini_max::MiniMax::CloseDatabase() {
}

void mini_max::MiniMax::PauseDatabaseCalculation() {
}

void mini_max::MiniMax::CancelDatabaseCalculation() {
}

bool mini_max::MiniMax::WasDatabaseCalculationCalcelled() {
}

unsigned int mini_max::MiniMax::GetLastCalculatedLayer() {
}

bool mini_max::MiniMax::SetOutputStream(std::wostream& the_stream) {
}

bool mini_max::MiniMax::SetNumThreads(unsigned int num_threads) {
}

bool mini_max::MiniMax::CalcLayer(unsigned int layer_number) {
}

void mini_max::MiniMax::SetCurrentActivity(Activity new_action) {
}
}  // namespace muehle
