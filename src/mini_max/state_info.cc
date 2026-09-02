#include "muehle/mini_max/state_info.h"

namespace muehle {
namespace mini_max {

void mini_max::StateInfo::UpdateBestAmountOfPlies() {
  /* When the game is won, then try to find the minimum amount of plies to win
   */
  if (short_value == SKV_VALUE_GAME_WON) {
    best_amount_of_plies = PLYINFO_VALUE_INVALID;
    for (auto& cur_choice : choices) {
      if (cur_choice.short_value == SKV_VALUE_GAME_WON) {
        if (best_amount_of_plies > cur_choice.ply_info + 1) {
          best_amount_of_plies = cur_choice.ply_info + 1;
        }
      }
    }
    /* When the game is lost, then try to find the maximum amount of plies to
     * lose */
  } else if (short_value == SKV_VALUE_GAME_LOST) {
    best_amount_of_plies = 0;
    for (auto& cur_choice : choices) {
      if (cur_choice.short_value == SKV_VALUE_GAME_LOST) {
        if (best_amount_of_plies < cur_choice.ply_info + 1) {
          best_amount_of_plies = cur_choice.ply_info + 1;
        }
      }
    }
    /* When the game is drawn, then try to maximuse the choice leading to a
     * state with the most options to win */
  } else if (short_value == SKV_VALUE_GAME_DRAWN) {
    best_amount_of_plies = 0;
    for (auto& cur_choice : choices) {
      if (cur_choice.short_value == SKV_VALUE_GAME_DRAWN) {
        if (best_amount_of_plies <=
            cur_choice.freq_values_sub_moves[SKV_VALUE_GAME_WON]) {
          best_amount_of_plies =
              cur_choice.freq_values_sub_moves[SKV_VALUE_GAME_WON];
        }
      }
    }
    /* Invalid state */
  } else {
    best_amount_of_plies = PLYINFO_VALUE_INVALID;
  }
}

}  // namespace mini_max
}  // namespace muehle
