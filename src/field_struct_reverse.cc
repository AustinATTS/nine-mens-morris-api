#include "muehle/field_struct_reverse.h"

#include "muehle/field_struct.h"

namespace muehle {

/* Returns the predecessors fields of the current fie */
void FieldStructReverse::GetPredecessors(
    std::vector<FieldStructTypes::Core>& pred_fields) const {
  /* The important variables which must be updated for the GetLyerAndStateNumber
   * function are 'field.cur_player.num_stones' 'field_cur_player.id',
   * 'field.field', and 'field.setting_hase' (Includes the opponent player
   * variants as well). */
  pred_fields.clear();

  /* Locals */
  FieldStructReverse tmp_field = *this;
  bool mill_was_closed = false;

  /* Stone was removed */
  GetPredecessors_stoneRemove(pred_fields, tmp_field);

  /* In moving phase */
  GetPredecessors_normalMove(pred_fields, tmp_field, mill_was_closed);

  /* In jumping phase */
  GetPredecessors_jumpingPhase(pred_fields, tmp_field, mill_was_closed);

  /* In setting phase */
  GetPredecessors_settingPhase(pred_fields, tmp_field, mill_was_closed);
}

/* Helper function to get the predecessors in the stone setting phase */
void FieldStructReverse::GetPredecessors_settingPhase(
    std::vector<FieldStructTypes::Core>& pred_fields, FieldStructReverse& field,
    bool mill_was_closed) const {
  /* Locals */
  unsigned int to;
  bool setting_phase_backup;
  bool game_has_finished_backup;

  /* Number of stones set must be at least 1 */
  if (field.opp_player.num_stones_set < 1 || field.opp_player.num_stones < 1) {
    return;
  }

  /* If already in moving phase, but coming from setting phase */
  if (!field.setting_phase) {
    /* Then both players must have at least 3 stones */
    if (field.cur_player.num_stones < 3 || field.opp_player.num_stones < 3) {
      return;
    }

    /* If a mill was closed then at least 3 stones must be present and a mill */
    if (mill_was_closed && field.cur_player.num_stones < 3 &&
        field.cur_player.number_of_mills > 0) {
      return;
    }

    /* Total number of stones must be correct */
    if (field.GetNumStonesSet() != field.cur_player.num_stones +
                                       field.opp_player.num_stones +
                                       field.cur_player.num_stones_missing +
                                       field.opp_player.num_stones_missing) {
      return;
    }

    /* All stones must be set to be in the moving phase */
    if (field.cur_player.num_stones_set != FieldStruct::num_stones_per_player ||
        field.opp_player.num_stones_set != FieldStruct::num_stones_per_player) {
      return;
    }

    /* Stone could have been placed anywhere */
    for (to = 0; to < field.size; to++) {
      /* Do not allow to close two mills at once */
      if (field.stone_part_of_mill[to] >= 2) {
        continue;
      }

      /* Stone which was set must be owned by the current player if a mill was
       * closed, otherwise it must be owned by the opponent */
      if (field.field[to] !=
          (mill_was_closed ? field.cur_player.id : field.opp_player.id)) {
        continue;
      }

      /* If a mill was closed so the stone must be part of a mill */
      if (mill_was_closed && field.stone_part_of_mill[to] == 0) {
        continue;
      }

      /* If no mill was closed so the stone must not be part of a mill */
      if (!mill_was_closed && field.stone_part_of_mill[to] != 0) {
        continue;
      }

      /* Remove stone set during this setting phase step */
      {
        setting_phase_backup = field.setting_phase;
        game_has_finished_backup = field.game_has_finished;
        field.setting_phase = true;
        field.game_has_finished = false;

        field.field[to] = PlayerId::square_is_free;
        if (mill_was_closed) {
          field.cur_player.num_stones--;
          field.cur_player.num_stones_set--;
          field.cur_player.number_of_mills--;
        } else {
          field.opp_player.num_stones--;
          field.opp_player.num_stones_set--;
          std::swap(/* a: */ field.cur_player, /* b: */ field.opp_player);
        }
      }

      StorePredecessor(pred_fields, field);

      /* Put stone back */
      {
        if (mill_was_closed) {
          field.field[to] = field.cur_player.id;
          field.cur_player.number_of_mills++;
          field.cur_player.num_stones_set++;
          field.cur_player.num_stones++;
        } else {
          std::swap(/* a: */ field.cur_player, /* b: */ field.opp_player);
          field.opp_player.num_stones_set++;
          field.opp_player.num_stones++;
          field.field[to] = field.opp_player.id;
        }
      }

      field.setting_phase = setting_phase_backup;
      field.game_has_finished = game_has_finished_backup;
    }
  }
}

/* Helper function to get the predecessors in the normal phase */
void FieldStructReverse::GetPredecessors_normalMove(
    std::vector<FieldStructTypes::Core>& pred_fields, FieldStructReverse& field,
    bool mill_was_closed) const {
  /* Locals */
  unsigned int from, to, dir;
  bool game_has_finished_backup;

  /* Game must not be in setting phase anymore */
  if (field.setting_phase) {
    return;
  }

  /* Both players must have atleast 3 stones and game must not be finished yet
   */
  if (field.cur_player.num_stones < 3 || field.opp_player.num_stones < 3 ||
      (field.game_has_finished && field.cur_player.num_possible_moves != 0)) {
    return;
  }

  /* Test each direction */
  for (to = 0; to < field.size; to++) {
    /* Stone which was moved must be owned by the current player, if a mill was
     * closed otherwise it must be owned by the opponent player */
    if (field.field[to] !=
        (mill_was_closed ? field.cur_player.id : field.opp_player.id)) {
      continue;
    }

    /* When stone is going to be removed than a mill must be closed */
    if (mill_was_closed && field.stone_part_of_mill[to] == 0) {
      continue;
    }

    /* When stone is part of a mill then a stone must be removed */
    if (!mill_was_closed && field.stone_part_of_mill[to] != 0) {
      continue;
    }

    /* Test each direction */
    for (dir = 0; dir < 4; dir++) {
      /* Origin */
      from = field.connected_square[to][dir];

      /* Move possible? */
      if (!(from < field.size &&
            field.field[from] == PlayerId::square_is_free)) {
        continue;
      }

      /* Make move */
      {
        if (mill_was_closed) {
          field.cur_player.number_of_mills--;
        } else {
          std::swap(/* a: */ field.cur_player, /* b: */ field.opp_player);
        }

        field.field[from] = field.field[to];
        field.field[to] = PlayerId::square_is_free;
        game_has_finished_backup = field.game_has_finished;
        field.game_has_finished = false;
      }

      StorePredecessor(pred_fields, field);

      /* Undo move */
      {
        field.field[to] = field.field[from];
        field.field[from] = PlayerId::square_is_free;
        field.game_has_finished = game_has_finished_backup;

        if (mill_was_closed) {
          field.cur_player.number_of_mills++;
        } else {
          std::swap(/* a: */ field.cur_player, /* b: */ field.opp_player);
        }
      }
    }
  }
}

/* Helper function to get the predecessors in the jumping phase */
void FieldStructReverse::GetPredecessors_jumpingPhase(
    std::vector<FieldStructTypes::Core>& pred_fields, FieldStructReverse& field,
    bool mill_was_closed) const {
  /* Locals */
  unsigned int from, to;

  /* Game must not be in setting phase anymore */
  if (field.setting_phase) {
    return;
  }

  /* Both players must have atleast 3 stones and game must not be finished yet
   */
  if (field.cur_player.num_stones < 3 || field.opp_player.num_stones < 3 ||
      field.game_has_finished) {
    return;
  }

  /* Test each direction */
  for (to = 0; to < field.size; to++) {
    /* When a stone must be removed, the current player must have closed a mill.
     * otherwise the opponent did a common spring move */
    if (field.field[to] !=
        (mill_was_closed ? field.cur_player.id : field.opp_player.id)) {
      continue;
    }

    /* When stone is going to be removed than a mill must be closed */
    if (mill_was_closed && field.stone_part_of_mill[to] == 0) {
      continue;
    }

    /* When stone is part of a mill then a stone must be removed */
    if (!mill_was_closed && field.stone_part_of_mill[to] != 0) {
      continue;
    }

    /* Test each stone origin */
    for (from = 0; from < field.size; from++) {
      /* Move possible? */
      {
        if (field.field[from] != PlayerId::square_is_free) {
          continue;
        }

        /* Is current player allowed to jump? */
        if (field.cur_player.num_stones > 3 && mill_was_closed ||
            field.opp_player.num_stones > 3 && !mill_was_closed) {
          /* Determine moving direction */
          unsigned int moving_direction = 4;
          for (unsigned int k = 0; k < 4; k++) {
            if (field.connected_square[from][k] == to) {
              moving_direction = k;
            }
          }

          /* Are both squares connected */
          if (moving_direction == 4) {
            continue;
          }
        }
      }

      /* Make move */
      {
        if (mill_was_closed) {
          field.cur_player.number_of_mills--;
        } else {
          std::swap(/* a: */ field.cur_player, /* b: */ field.opp_player);
        }

        field.field[from] = field.field[to];
        field.field[to] = PlayerId::square_is_free;
      }

      StorePredecessor(pred_fields, field);

      /* Undo move */
      {
        field.field[to] = field.field[from];
        field.field[from] = PlayerId::square_is_free;

        if (mill_was_closed) {
          field.cur_player.number_of_mills++;
        } else {
          std::swap(/* a: */ field.cur_player, /* b: */ field.opp_player);
        }
      }
    }
  }
}

/* Helper function to get the predecessors in the remove phase */
void FieldStructReverse::GetPredecessors_stoneRemove(
    std::vector<FieldStructTypes::Core>& pred_fields,
    FieldStructReverse& field) const {
  /* Locals */
  unsigned int from;
  bool game_has_finished_backup;
  unsigned int stone_from_mill_was_removed;

  /* A stone was only removed when the current player has less than 9 stones and
   * at least 1 stone missing */
  if (field.cur_player.num_stones <= 9 ||
      field.cur_player.num_stones_missing == 0 ||
      field.cur_player.num_stones_set == 0) {
    return;
  }

  /* At least 5 stones must be set (3 for the opponent mill, 2 for the current
   * player (1 being removed)) */
  if (field.GetNumStonesSet() < 5 && field.opp_player.num_stones_set < 3 &&
      field.cur_player.num_stones < 1) {
    return;
  }

  /* Opponent must have a closed mill */
  if (!field.opp_player.number_of_mills) {
    return;
  }

  /* From each free position the opponent could have removed a stone from the
   * current player */
  for (from = 0; from < field.size; from++) {
    /* Square free? */
    if (field.field[from] != PlayerId::square_is_free) {
      continue;
    }

    /* Stone mustnt be part of a mill, except if player has only mills */
    stone_from_mill_was_removed = 0;
    {
      std::vector<FieldPos> mill_stones = {
          from, field.neighbour[from][0][0], field.neighbour[from][0][1],
          field.neighbour[from][1][0], field.neighbour[from][1][1]};
      if (field.field[mill_stones[1]] == field.cur_player.id &&
          field.field[mill_stones[2]] == field.cur_player.id) {
        stone_from_mill_was_removed++;
      }
      if (field.field[mill_stones[3]] == field.cur_player.id &&
          field.field[mill_stones[4]] == field.cur_player.id) {
        stone_from_mill_was_removed++;
      }
      if (stone_from_mill_was_removed && AnyLonelyStone(field, from)) {
        continue;
      }
    }

    /* If stone was removed from mill, then player must have at least one stone
     * on the board */
    if (stone_from_mill_was_removed) {
      if (field.cur_player.num_stones == 0 ||
          field.cur_player.num_stones_set == 0) {
        continue;
      }
    }
    /* Do not allow stone removal when its a part of two mills */
    if (stone_from_mill_was_removed > 1) {
      continue;
    }

    /* Put back stone */
    {
      game_has_finished_backup = field.game_has_finished;
      field.game_has_finished = false;
      field.field[from] = field.cur_player.id;
      field.cur_player.num_stones++;
      field.cur_player.num_stones_missing--;
      if (stone_from_mill_was_removed) {
        field.cur_player.number_of_mills += stone_from_mill_was_removed;
        field.cur_player.has_only_mills = true;
        field.CalcStonePartOfMill();
      }
      std::swap(/* a: */ field.cur_player, /* b: */ field.opp_player);
    }

    /* Get predecessor from closing the mill */
    GetPredecessors_normalMove(pred_fields, field, /* mill_was_closed: */ true);
    GetPredecessors_jumpingPhase(pred_fields, field,
                                 /* mill_was_closed: */ true);
    GetPredecessors_settingPhase(pred_fields, field,
                                 /* mill_was_closed: */ true);

    /* Remove stone again */
    {
      std::swap(/* a: */ field.cur_player, /* b: */ field.opp_player);
      field.field[from] = PlayerId::square_is_free;
      field.game_has_finished = game_has_finished_backup;
      field.cur_player.num_stones--;
      field.cur_player.num_stones_missing++;
      if (stone_from_mill_was_removed) {
        field.cur_player.number_of_mills -= stone_from_mill_was_removed;
        field.cur_player.has_only_mills = false;
        field.CalcStonePartOfMill();
      }
    }
  }
}

/* Store the current field state as a predecessor */
bool FieldStructReverse::StorePredecessor(
    std::vector<FieldStructTypes::Core>& pred_fields,
    const FieldStructReverse& field) const {
  /* Store predecessor */
  if (field.IsIntegrityOk()) {
    pred_fields.push_back(field);
    return true;
  } else {
    /* TODO: The conditions should be checked within the GetPredecessor
     * functions, such that IsIntegrity should not be necessary assert(false) */
    return false;
  }
}

/* Checks if there is any lonely stone, not being part of a mill. A stone is
 * considered lonely if it is not part of any mill. 'removed_from' is the
 * position of the stone that was removed from the field. */
bool FieldStructReverse::AnyLonelyStone(const FieldStructReverse& field,
                                        FieldPos removed_from) const {
  /* Check every stone */
  for (FieldPos k = 0; k < field.size; k++) {
    /* Skip the current removed stone and the potential mills being closed by
     * that stone */
    if (k == removed_from) {
      continue;
    }
    if (k == field.neighbour[removed_from][0][1] &&
        field.field[field.neighbour[removed_from][0][0]] ==
            field.cur_player.id) {
      continue;
    }
    if (k == field.neighbour[removed_from][0][0] &&
        field.field[field.neighbour[removed_from][0][1]] ==
            field.cur_player.id) {
      continue;
    }
    if (k == field.neighbour[removed_from][1][1] &&
        field.field[field.neighbour[removed_from][1][0]] ==
            field.cur_player.id) {
      continue;
    }
    if (k == field.neighbour[removed_from][1][0] &&
        field.field[field.neighbour[removed_from][1][1]] ==
            field.cur_player.id) {
      continue;
    }

    /* Check if the stone is lonely */
    if (field.field[k] == field.cur_player.id && !field.stone_part_of_mill[k]) {
      return true;
    }
  }
  return false;
}

} /* namespace muehle */
