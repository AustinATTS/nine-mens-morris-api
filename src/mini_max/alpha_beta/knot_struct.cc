#include "muehle/mini_max/alpha_beta/knot_struct.h"

namespace muehle {

/* Initialise knot for calculation */
bool mini_max::alpha_beta::KnotStruct::InitForCaclulation(
    KnotStruct* branch_array) {
  branches = branch_array;
  num_possibilities = 0;
  best_move_id = 0;
  ply_info = PLYINFO_VALUE_UNCALCULATED;
  short_value = SKV_VALUE_GAME_DRAWN;
  float_value = skv_float_value_map[short_value];
  freq_values_sub_moves[SKV_VALUE_INVALID] = 0;
  freq_values_sub_moves[SKV_VALUE_GAME_LOST] = 0;
  freq_values_sub_moves[SKV_VALUE_GAME_DRAWN] = 0;
  freq_values_sub_moves[SKV_VALUE_GAME_WON] = 0;
  return true;
}

/* Set knot to invalid state */
void mini_max::alpha_beta::KnotStruct::SetInvalid() {
  short_value = SKV_VALUE_INVALID;
  ply_info = SKV_VALUE_INVALID;
  float_value = SKV_VALUE_INVALID;
}

/* Calculates ply_info of knot based on branches
 * Required:
 * - branches[i].short_value
 * - branches[i].ply_info
 * - branches[i].player_to_move_changed
 * - num_possibilities
 * - short_value
 * Output: ply_info */
bool mini_max::alpha_beta::KnotStruct::CalcPlyInfo() {
  /* Checks */
  if (num_possibilities == 0) {
    return false;
  }
  if (branches == nullptr) {
    return false;
  }

  /* Locals */
  unsigned int max_branch;
  PlyInfoVarType max_ply_info;

  /* Ignore invalid and drawn states */
  if (short_value == SKV_VALUE_GAME_DRAWN) {
    ply_info = PLYINFO_VALUE_DRAWN;
  } else if (short_value == SKV_VALUE_INVALID) {
    ply_info = PLYINFO_VALUE_INVALID;
  } else {
    /* Calculate ply info of knot */
    max_ply_info =
        (short_value == SKV_VALUE_GAME_WON) ? PLYINFO_VALUE_DRAWN : 0;
    max_branch = 0;

    /* When current knot is a won state */
    if (short_value == SKV_VALUE_GAME_WON) {
      for (unsigned int i = 0; i < num_possibilities; i++) {
        /* Take the minimum ply of the lost states, when the opponent moved last
         */
        if ((branches[i].ply_info < max_ply_info &&
             branches[i].short_value == SKV_VALUE_GAME_LOST &&
             branches[i].player_to_move_changed) ||
            (branches[i].ply_info < max_ply_info &&
             branches[i].short_value == SKV_VALUE_GAME_WON &&
             !branches[i].player_to_move_changed)) {
          /* After this move, the same player will continue, so take the minimum
           * ply of the won state */
          max_ply_info = branches[i].ply_info;
          max_branch = i;
        }
      }

      /* Current state is a lost state */
    } else {
      for (unsigned int i = 0; i < num_possibilities; i++) {
        /* Take the maximum of the won states (won for the opponent), since
         * that's the longest path for them */
        if ((branches[i].ply_info > max_ply_info &&
             branches[i].short_value == SKV_VALUE_GAME_WON &&
             branches[i].player_to_move_changed) ||
            (branches[i].ply_info > max_ply_info &&
             branches[i].short_value == SKV_VALUE_GAME_LOST &&
             !branches[i].player_to_move_changed)) {
          /* After this move, the same player will continue, so take the minimum
           * ply of the lost state */
          max_ply_info = branches[i].ply_info;
          max_branch = i;
        }
      }
    }

    /* Set value */
    ply_info = branches[max_branch].ply_info + 1;
  }

  return true;
}

/* Calculates float_value and short_value of knot based on branches
 * Required:
 * - branches[i].short_value
 * - branches[i].float_value
 * - branches[i].player_to_move_changed
 * - num_possibilities
 * Output: float_value
 */
bool mini_max::alpha_beta::KnotStruct::CalcKnotValue() {
  /* Checks */
  if (num_possibilities == 0) {
    return false;
  }
  if (branches == nullptr) {
    return false;
  }

  /* Locals */
  short_value = SKV_VALUE_INVALID;
  float_value = FPKV_INV_VALUE;
  TwoBit skv;
  float fv;
  unsigned int cur_poss;

  /* Maximise the value */
  for (cur_poss = 0; cur_poss < num_possibilities; cur_poss++) {
    /* Do not create invalid states */
    if (branches[cur_poss].short_value == SKV_VALUE_INVALID) {
      continue;
    }

    /* Get perspective corrective value from the current considered branch */
    if (branches[cur_poss].player_to_move_changed) {
      skv = skv_perspective_matrix[branches[cur_poss].short_value]
                                  [PL_TO_MOVE_CHANGED];
      fv = -1.0f * branches[cur_poss]
                       .float_value; /* Due to this negation the float_value
                                        must be symmetric around 0 */
    } else {
      skv = branches[cur_poss].short_value;
      fv = branches[cur_poss].float_value;
    }

    /* Maximize skv */
    if (skv > short_value) {
      short_value = skv;
    }

    /* Maximize float value */
    if (fv > float_value) {
      float_value = fv;
    }
  }

  return true;
}

/* Select randomly one of the best moves, if they are equivalent
 * Required:
 * - branches[i].short_value
 * - branches[i].ply_info
 * - branches[i].player_to_move_changed
 * - num_possibilities
 * - short_value
 * Output: best_branches: indices of the best */
bool mini_max::alpha_beta::KnotStruct::GetBestBranchesBasedOnSkvValue(
    std::vector<unsigned int>& best_branches) {
  /* Checks */
  if (num_possibilities == 0) {
    return false;
  }
  if (branches == nullptr) {
    return false;
  }

  best_branches.clear();
  best_branches.reserve(num_possibilities);

  /* Check every possible move */
  for (unsigned int cur_poss = 0; cur_poss < num_possibilities; cur_poss++) {
    /* Value of selected move is equal to knot value */
    TwoBit branch_skv =
        (branches[cur_poss].player_to_move_changed
             ? skv_perspective_matrix[branches[cur_poss].short_value]
                                     [PL_TO_MOVE_CHANGED]
             : branches[cur_poss].short_value);
    if (branch_skv == short_value) {
      /* Best move lead to drawn state */
      if (short_value == SKV_VALUE_GAME_DRAWN) {
        best_branches.push_back(cur_poss);

        /* Best move lead to lost or win state */
      } else {
        if (ply_info == branches[cur_poss].ply_info + 1) {
          best_branches.push_back(cur_poss);
        }
      }
    }
  }

  return true;
}

/* Select randomly one of the best moves, if they are equivalent
 * Required:
 * - use_skv_value: true if skv_value should be used, false if float_value
 * - branches[i].float_value
 * - branches[i].player_to_move_changed
 * - num_possibilities
 * - float_value
 * Output: best_branches: indices of the best */
bool mini_max::alpha_beta::KnotStruct::GetBestBranchesBasedOnFloatValue(
    std::vector<unsigned int>& best_branches) {
  /* Checks */
  if (num_possibilities == 0) {
    return false;
  }
  if (branches == nullptr) {
    return false;
  }

  best_branches.reserve(num_possibilities);

  /* Check every possible move */
  for (unsigned int cur_poss = 0; cur_poss < num_possibilities; cur_poss++) {
    /* Skip branches leading to invalid states */
    if (branches[cur_poss].float_value <= FPKV_INV_VALUE + FPKV_THRESHOLD) {
      continue;
    }

    /* Conventional Mini Max algorithm */
    float dif;
    float f_player_to_move_changed =
        (branches[cur_poss].player_to_move_changed) ? -1.0f : 1.0f;
    dif =
        f_player_to_move_changed * branches[cur_poss].float_value - float_value;
    dif = (dif > 0) ? dif : -1.0f * dif;
    if (dif < FPKV_THRESHOLD) {
      best_branches.push_back(cur_poss);
    }
  }

  return true;
}

/* Filles info_about_choices with information about choices
 * Required:
 * - possibility_ids
 * - branches[i].short_value
 * - branches[i].ply_info
 * - branches[i].player_to_move_changed
 * - branches[i].freq_values_sub_moves
 * - num_possibilities
 * - short_value
 * Output: info_about_choices
 */
bool mini_max::alpha_beta::KnotStruct::GetInfoAboutChoices(
    StateInfo& info_about_choices) {
  info_about_choices.ply_info = ply_info;
  info_about_choices.short_value = short_value;
  info_about_choices.choices.resize(possibility_ids.size());
  for (unsigned int i = 0; i < possibility_ids.size(); i++) {
    info_about_choices.choices[i].possibility_id = possibility_ids[i];
    info_about_choices.choices[i].short_value =
        (branches[i].player_to_move_changed
             ? skv_perspective_matrix[branches[i].short_value]
                                     [PL_TO_MOVE_CHANGED]
             : branches[i].short_value);
    info_about_choices.choices[i].ply_info = branches[i].ply_info;
    info_about_choices.choices[i].freq_values_sub_moves[SKV_VALUE_INVALID] =
        branches[i].freq_values_sub_moves[SKV_VALUE_INVALID];
    info_about_choices.choices[i].freq_values_sub_moves[SKV_VALUE_GAME_LOST] =
        branches[i].freq_values_sub_moves[SKV_VALUE_GAME_LOST];
    info_about_choices.choices[i].freq_values_sub_moves[SKV_VALUE_GAME_DRAWN] =
        branches[i].freq_values_sub_moves[SKV_VALUE_GAME_DRAWN];
    info_about_choices.choices[i].freq_values_sub_moves[SKV_VALUE_GAME_WON] =
        branches[i].freq_values_sub_moves[SKV_VALUE_GAME_WON];
  }
  return true;
}

/* Increase freq_values_sub_moves according to knot value of the chose branch by
 * 'cur_poss
 * Required:
 * - branches[i].short_value
 * - num_possibilities
 * Outputs: freq_value_sub_moves */
bool mini_max::alpha_beta::KnotStruct::IncreaseFreqValuesSubMoves(
    unsigned int cur_poss) {
  if (cur_poss >= num_possibilities) {
    return false;
  }
  TwoBit skv = (branches[cur_poss].player_to_move_changed
                    ? skv_perspective_matrix[branches[cur_poss].short_value]
                                            [PL_TO_MOVE_CHANGED]
                    : branches[cur_poss].short_value);
  freq_values_sub_moves[skv]++;
  return true;
}

/* Returns true if a cut off (ignoring furthur possible moves) can be used */
bool mini_max::alpha_beta::KnotStruct::CanCutOff(unsigned int cur_poss,
                                                 float& alpha, float& beta) {
  if (!branches[cur_poss].player_to_move_changed) {
    if (branches[cur_poss].float_value >= beta) {
      num_possibilities = cur_poss + 1;
      return true;
    } else if (branches[cur_poss].float_value > alpha) {
      alpha = branches[cur_poss].float_value;
    }
  } else {
    if (branches[cur_poss].float_value <= alpha) {
      num_possibilities = cur_poss + 1;
      return true;
    } else if (branches[cur_poss].float_value < beta) {
      beta = branches[cur_poss].float_value;
    }
  }
  return false;
}

}  // namespace muehle
