#ifndef MUEHLE_MINI_MAX_ALPHA_BETA_KNOT_STRUCT_H_
#define MUEHLE_MINI_MAX_ALPHA_BETA_KNOT_STRUCT_H_

#include "muehle/mini_max/state_info.h"
#include "muehle/mini_max/type_def.h"

namespace muehle {
namespace mini_max {
namespace alpha_beta {

/* This represents a state of the game */
struct KnotStruct {
  bool player_to_move_changed = true; /* The player to move switched compared to
                                         the parent/origin knot/state */
  float float_value =
      0.0f; /* Value of knot (for normal mode). Must be symmetric around 0 from
               the view of the current player */
  TwoBit short_value = SKV_VALUE_INVALID; /* Value of knot (for database) */
  unsigned int best_move_id = 0;          /* For calling class */
  unsigned int num_possibilities =
      0; /* Number of branches - differs from possibility_ids.size() in case of
            cut off */
  PlyInfoVarType ply_info = 0; /* Number of moves till win/lose */
  KnotStruct* branches =
      nullptr; /* Pointer to branches, in sync with possibility_ids */
  unsigned int freq_values_sub_moves[SKV_NUM_VALUES] = {
      0, 0, 0, 0}; /* Number of branches leading to a state with a certain
                      value, from the perspective of the current player */
  std::vector<unsigned int>
      possibility_ids; /* Filled by game->GetPossibilities(); contains IDs for
                          all possible moves, while 'branches' points to the
                          corresponding KnotStructs for moves that are actually
                          explored (may differ in size if cutoffs occur) */

  bool InitForCaclulation(KnotStruct* branch_array);
  void SetInvalid();
  bool CalcPlyInfo();
  bool CalcKnotValue();
  bool GetBestBranchesBasedOnSkvValue(std::vector<unsigned int>& best_branches);
  bool GetBestBranchesBasedOnFloatValue(
      std::vector<unsigned int>& best_branches);
  bool GetInfoAboutChoices(StateInfo& info_about_choices);
  bool IncreaseFreqValuesSubMoves(unsigned int cur_poss);
  bool CanCutOff(unsigned int cur_poss, float& alpha, float& beta);
};

}  // namespace alpha_beta
}  // namespace mini_max
}  // namespace muehle

#endif  // MUEHLE_MINI_MAX_ALPHA_BETA_KNOT_STRUCT_H_
