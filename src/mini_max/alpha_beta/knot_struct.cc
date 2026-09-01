#include "muehle/mini_max/alpha_beta/knot_struct.h"

namespace muehle {

// Implementation goes here.

bool mini_max::alpha_beta::KnotStruct::InitForCaclulation(
    KnotStruct* branch_array) {
}

void mini_max::alpha_beta::KnotStruct::SetInvalid() {
}

bool mini_max::alpha_beta::KnotStruct::CalcPlyInfo() {
}

bool mini_max::alpha_beta::KnotStruct::CalcKnotValue() {
}

bool mini_max::alpha_beta::KnotStruct::GetBestBranchesBasedOnSkvValue(
    std::vector<unsigned int>& best_branches) {
}

bool mini_max::alpha_beta::KnotStruct::GetBestBranchesBasedOnFloatValue(
    std::vector<unsigned int>& best_branches) {
}

bool mini_max::alpha_beta::KnotStruct::GetInfoAboutChoices(
    StateInfo& info_about_choices) {
}

bool mini_max::alpha_beta::KnotStruct::IncreaseFreqValuesSubMoves(
    unsigned int cur_poss) {
}

bool mini_max::alpha_beta::KnotStruct::CanCutOff(unsigned int cur_poss,
    float& alpha, float& beta) {
}
}  // namespace muehle
