#include "muehle/api/muehle_bridge.h"

#include <exception>
#include <iostream>

namespace muehle {

MuehleBridge::MuehleBridge() : game(), ai() {}

void MuehleBridge::SetSearchDepth(unsigned int depth) {
  search_depth = depth;
  ai.SetSearchDepth(search_depth);
}

MuehleBridge::Response MuehleBridge::Evaluate(const Request& req) {
  Response result;
  result.search_depth = search_depth;
  result.current_player = req.current_player;
  result.setting_phase = req.setting_phase;
  result.total_num_stones_missing = req.total_num_stones_missing;

  /* Attempt to open database */
  const bool db_loaded = ai.OpenDatabase(/* directory: */ L"./database/");

  if (db_loaded) {
    result.engine = "perfect-database";
  } else {
    /* Log fallback warning */
    std::cerr << "[MuehleBridge Warning] Database missing or unreadable at "
                 "./database/. "
              << "Falling back to AlphaBeta engine search (Depth: "
              << search_depth << ").\n";
    result.engine = "alpha-beta";
  }

  /* Set up board situation */
  FieldStruct field;
  field.Reset(req.current_player);
  if (!field.SetSituation(req.board, req.setting_phase,
                          req.total_num_stones_missing)) {
    result.success = false;
    result.error = "Invalid board state for the supplied player and phase. ";
    return result;
  }

  /* Evaluate position (MinMaxAi handles AB fallback internally if db is closed
   */
  MoveInfo best_move;
  try {
    ai.SetSearchDepth(search_depth);
    ai.Play(field, best_move);
  } catch (const std::exception& ex) {
    result.success = false;
    result.error = ex.what();
    return result;
  }

  result.success = true;
  result.best_move = best_move;
  result.choice_info = ai.GetInfoAboutChoices();
  result.game_has_finished = field.HasGameFinished();
  result.winner = field.GetWinner();

  for (const auto& choice : result.choice_info.choices) {
    ChoiceResult output_choice;
    output_choice.possibility_id = choice.possibility_id;
    output_choice.move.SetId(choice.possibility_id);
    output_choice.short_value = choice.short_value;
    output_choice.ply_info = choice.ply_info;
    for (unsigned int value_index = 0; value_index < mini_max::SKV_NUM_VALUES;
         value_index++) {
      output_choice.freq_values_sub_moves[value_index] =
          choice.freq_values_sub_moves[value_index];
    }
    result.choices.push_back(output_choice);
  }

  return result;
}

} /* namespace muehle */
