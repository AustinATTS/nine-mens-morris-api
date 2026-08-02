#include "muehle/field_struct_forward.h"

#include <cassert>

namespace muehle {
using namespace std;

/* Returns the possible moves for the current player */
void FieldStructForward::GetPossibilities(
    std::vector<MoveInfo::PossibilityId>& possibility_ids) const {
  /* When game has ended, nothing happens anymore */
  if (game_has_finished || !IsIntegrityOk()) {
    possibility_ids.clear();
    /* Look what to do */
  } else {
    if (setting_phase) {
      GetPossSettingPhase(possibility_ids);
    } else {
      GetPossNormalMove(possibility_ids);
    }
  }
}

/* Helper function to get the possible moves in the setting phase */
void FieldStructForward::GetPossSettingPhase(
    std::vector<MoveInfo::PossibilityId>& possibility_ids) const {
  /* Locals */
  FieldPos to;
  unsigned int number_of_mills_being_closed;
  vector<FieldPos> removable_stones;

  /* get all removable stones */
  GetPossStoneRemove(removable_stones);

  /* Clear possibilities */
  possibility_ids.clear();

  /* Possibilities with cut off */
  for (to = 0; to < size; to++) {
    /* Move possible? */
    if (field[to] != PlayerId::square_is_free) {
      continue;
    }

    /* Check if a mill is being closed */
    number_of_mills_being_closed = WouldMillBeClosed(FieldStruct::size, to);

    /* If a mill is closed, generate moves with stone removal. Don't allow to
     * close two mills at once, don't allow to close a mill, although no stone
     * can be removed from the opponent */
    if (number_of_mills_being_closed == 1 && removable_stones.size()) {
      for (FieldPos remove_pos : removable_stones) {
        possibility_ids.push_back(MoveInfo{size, to, remove_pos}.GetId());
      }
      /* No mill closed, generate move without stone removal */
    } else if (number_of_mills_being_closed == 0) {
      possibility_ids.push_back(MoveInfo{size, to, size}.GetId());
    }
  }
}

/* Helper function to get the possible moves in the normal phase */
void FieldStructForward::GetPossNormalMove(
    std::vector<MoveInfo::PossibilityId>& possibility_ids) const {
  /* Locals */
  FieldPos from, to, dir, remove_pos;
  vector<FieldPos> removable_stones;

  possibility_ids.clear();

  /* Get all removalbe stones */
  GetPossStoneRemove(removable_stones);

  /* If he is not allowed to jump */
  if (cur_player.num_stones > 3) {
    for (from = 0; from < size; from++) {
      for (dir = 0; dir < 4; dir++) {
        /* Destination */
        to = connected_square[from][dir];

        /* Move possible? */
        if (to < size && field[from] == cur_player.id &&
            field[to] == PlayerId::square_is_free) {
          /* If a mill is closed, generate moves with stone removal */
          if (WouldMillBeClosed(from, to) && !removable_stones.empty()) {
            for (FieldPos remove_pos : removable_stones) {
              possibility_ids.push_back(MoveInfo{from, to, remove_pos}.GetId());
            }
            /* No mill closed, generate move without stone removal */
          } else {
            possibility_ids.push_back(MoveInfo{from, to, size}.GetId());
          }

          /* Current person is allowed to jump */
        }
      }
    }
  } else if (cur_player.num_stones == 3) {
    for (from = 0; from < size; from++) {
      for (to = 0; to < size; to++) {
        /* Move possilbe */
        if (field[from] == cur_player.id &&
            field[to] == PlayerId::square_is_free) {
          /* If a mill is closed, generate moves with stone removal */
          if (WouldMillBeClosed(from, to) && !removable_stones.empty()) {
            for (FieldPos remove_pos : removable_stones) {
              possibility_ids.push_back(MoveInfo{from, to, remove_pos}.GetId());
            }
            /* No mill closed, generate move without stone removal */
          } else {
            possibility_ids.push_back(MoveInfo{from, to, size}.GetId());
          }
        }
      }
    }
  } else {
    /* No possible moves */
  }

  assert(possibility_ids.size() < FieldStruct::max_number_pos_moves);
}

/* Helper function to get the possible moves to remove a stone */
void FieldStructForward::GetPossStoneRemove(
    std::vector<FieldPos>& removable_stones) const {
  /* Locals */
  FieldPos from;

  removable_stones.clear();

  /* Possibilities with cut off */
  for (from = 0; from < size; from++) {
    /* Move possible? */
    if (CanStoneBeRemoved(from)) {
      removable_stones.push_back(from);
    }
  }
}

/* Checks if a mill would be closed by moving from 'from' to 'to' does not
 * return true if the stone is already part of a mill at 'from' */
unsigned int FieldStructForward::WouldMillBeClosed(FieldPos from,
                                                   FieldPos to) const {
  unsigned int number_of_mills_being_closed = 0;

  /* Checks if a mill is being closed */
  if (cur_player.id == field[neighbour[to][0][0]] &&
      cur_player.id == field[neighbour[to][0][1]] &&
      neighbour[to][0][0] != from && neighbour[to][0][1] != from) {
    number_of_mills_being_closed++;
  }
  if (cur_player.id == field[neighbour[to][1][0]] &&
      cur_player.id == field[neighbour[to][1][1]] &&
      neighbour[to][1][0] != from && neighbour[to][1][1] != from) {
    number_of_mills_being_closed++;
  }

  /* Return true if a mill would be closed */
  return number_of_mills_being_closed;
}

/* Checks if a stone can be removed */
bool FieldStructForward::CanStoneBeRemoved(FieldPos pos) const {
  /* Check if the position is valid */
  if (pos >= size) {
    return false;
  }

  /* Check if the stone belongs to the opponent */
  if (field[pos] != opp_player.id) {
    return false;
  }

  /* If stone is not part of a mill then it can be removed */
  if (!stone_part_of_mill[pos]) {
    return true;
  }

  /* Do not allow to remove a stone belonging to two mills */
  if (stone_part_of_mill[pos] > 1) {
    return false;
  }

  /* If stone is part of a mill then it can only be removed if the opponent has
   * no 'free' stones */
  if (stone_part_of_mill[pos] > 0 && opp_player.has_only_mills) {
    return true;
  }

  /* If this is the last to be set during the setting phase, then it can be
   * removed */
  if (opp_player.has_only_mills && GetNumStonesSet() == 17) {
    return true;
  }

  /* Otherwise the stone cannot be removed */
  return false;
}

/* Updates 'stone_part_of_mill' */
void FieldStructForward::UpdateStonePartOfMill(FieldPos stone_one,
                                               FieldPos stone_two,
                                               FieldPos stone_three,
                                               PlayerId acting_player) {
  /* If all 3 fields are occupied by current player than he closed a mill */
  if (field[stone_one] == acting_player && field[stone_two] == acting_player &&
      field[stone_three] == acting_player) {
    stone_part_of_mill[stone_one]++;
    stone_part_of_mill[stone_two]++;
    stone_part_of_mill[stone_three]++;
  }

  /* Is a mill destroyed? */
  if (stone_part_of_mill[stone_one] && stone_part_of_mill[stone_two] &&
      stone_part_of_mill[stone_three] &&
      field[stone_one] == PlayerId::square_is_free &&
      field[stone_two] == acting_player &&
      field[stone_three] == acting_player) {
    stone_part_of_mill[stone_one]--;
    stone_part_of_mill[stone_two]--;
    stone_part_of_mill[stone_three]--;
  }
}

/* Helper function to call UpdateStonePartOfMill */
void FieldStructForward::UpdateWarning(FieldPos first_stone,
                                       FieldPos second_stone,
                                       PlayerId acting_player) {
  if (first_stone < size) {
    UpdateStonePartOfMill(first_stone, neighbour[first_stone][0][0],
                          neighbour[first_stone][0][1], acting_player);
  }
  if (first_stone < size) {
    UpdateStonePartOfMill(first_stone, neighbour[first_stone][1][0],
                          neighbour[first_stone][1][1], acting_player);
  }
  if (second_stone < size) {
    UpdateStonePartOfMill(second_stone, neighbour[second_stone][0][0],
                          neighbour[second_stone][0][1], acting_player);
  }
  if (second_stone < size) {
    UpdateStonePartOfMill(second_stone, neighbour[second_stone][1][0],
                          neighbour[second_stone][1][1], acting_player);
  }
}

/* Updates the number of possible moves for each player */
void FieldStructForward::UpdatePossibleMoves(FieldPos stone,
                                             PlayerStruct& stone_owner,
                                             bool stone_removed,
                                             FieldPos ignore_stone) {
  /* Locals */
  FieldPos neighbour, direction;

  /* Look in every direction */
  for (direction = 0; direction < 4; direction++) {
    neighbour = connected_square[stone][direction];

    /* Neihbour must exist */
    if (neighbour < size) {
      /* Relevent when moving from one square to another connected square */
      if (ignore_stone == neighbour) {
        continue;
      }

      /* If there is no neighbour stone than it only affects the actual stone */
      if (field[neighbour] == PlayerId::square_is_free) {
        if (stone_removed) {
          stone_owner.num_possible_moves--;
        } else {
          stone_owner.num_possible_moves++;
        }
        /* If there is a neigbour stone than it affects only this one */
      } else if (field[neighbour] == cur_player.id) {
        if (stone_removed) {
          cur_player.num_possible_moves++;
        } else {
          cur_player.num_possible_moves--;
        }
      } else {
        if (stone_removed) {
          opp_player.num_possible_moves++;
        } else {
          opp_player.num_possible_moves--;
        }
      }
    }
  }

  /* Only 3 stones resting */
  if (cur_player.num_stones == 3 && !setting_phase) {
    cur_player.num_possible_moves =
        cur_player.num_stones *
        (size - cur_player.num_stones - opp_player.num_stones);
  }
  if (opp_player.num_stones == 3 && !setting_phase) {
    opp_player.num_possible_moves =
        opp_player.num_stones *
        (size - cur_player.num_stones - opp_player.num_stones);
  }
  if (cur_player.num_stones < 3) {
    cur_player.num_possible_moves = 0;
  }
  if (opp_player.num_stones < 3) {
    opp_player.num_possible_moves = 0;
  }
}

/* Performs a move in the setting phase */
bool FieldStructForward::SetStone(const MoveInfo& move, BackupStruct& backup) {
  /* perameter ok */
  if (move.to >= size) {
    return false;
  }

  /* Is destination free? */
  if (field[move.to] != PlayerId::square_is_free) {
    return false;
  }

  /* Check if removal of stone is correct */
  if (move.remove_stone < size) {
    if (!CanStoneBeRemoved(move.remove_stone)) {
      return false;
    }
  }

  /* Set stone into field */
  field[move.to] = cur_player.id;
  cur_player.num_stones++;
  cur_player.num_stones_set++;

  /* Setting phase finished? */
  if (cur_player.num_stones_set + opp_player.num_stones_set == 18) {
    setting_phase = false;
  }

  /* Update possible moves */
  if (setting_phase) {
    cur_player.num_possible_moves--;
    opp_player.num_possible_moves--;
  } else {
    CalcNumPossibleMoves(cur_player);
    CalcNumPossibleMoves(opp_player);
  }

  /* Update warnings */
  UpdateWarning(move.to, size, cur_player.id);

  /* Handle stone removal if mill was closed */
  if (move.remove_stone < size) {
    RemoveStone(move, backup);
  }

  /* Calculate number of mills */
  CalcNumberOfMills();

  /* Everything is ok */
  return true;
}

/* Performs a normal move */
bool FieldStructForward::NormalMove(const MoveInfo& move,
                                    BackupStruct& backup) {
  /* Check if move is possible */
  if (move.from >= size) {
    return false;
  }
  if (move.to >= size) {
    return false;
  }
  if (field[move.from] != cur_player.id) {
    return false;
  }
  if (field[move.to] != PlayerId::square_is_free) {
    return false;
  }

  /* Check if removal of stone is correct */
  if (move.remove_stone < size) {
    if (!CanStoneBeRemoved(move.remove_stone)) return false;
  }

  /* Set stone into field */
  field[move.from] = PlayerId::square_is_free;
  field[move.to] = cur_player.id;

  /* Update possible moves */
  UpdatePossibleMoves(move.from, cur_player, true, move.to);
  UpdatePossibleMoves(move.to, cur_player, false, move.from);

  /* Update warnings */
  UpdateWarning(move.from, move.to, cur_player.id);
  CalcNumberOfMills();

  /* Handle stone removal if a mill was closed */
  if (move.remove_stone < size) {
    RemoveStone(move, backup);
  }

  /* Everything is ok */
  return true;
}

/* Removes a stone from the field */
bool FieldStructForward::RemoveStone(const MoveInfo& move,
                                     BackupStruct& backup) {
  /* Check if removal of stone is correct */
  if (!CanStoneBeRemoved(move.remove_stone)) {
    return false;
  }

  /* Remove stone */
  field[move.remove_stone] = PlayerId::square_is_free;
  opp_player.num_stones--;
  opp_player.num_stones_missing++;

      /* Update possible moves */
      if (setting_phase) {
    cur_player.num_possible_moves++;
    opp_player.num_possible_moves++;
  }
  else {
    UpdatePossibleMoves(move.remove_stone, opp_player, true, size);
  }

  /* Update warnings */
  UpdateWarning(move.remove_stone, size, opp_player.id);

  /* End of game? */
  if ((opp_player.num_stones < 3) && (!setting_phase)) {
    game_has_finished = true; /* Oppenent has less than 3 stones */
  }
  if ((!opp_player.num_possible_moves) && (!setting_phase) &&
      (opp_player.num_stones > 3)) {
    game_has_finished =
        true;  // Opponent has no possible moves and more than 3 stones
  }

  /* Everything is ok */
  return true;
}

/* Performs a move */
bool FieldStructForward::Move(const MoveInfo& move, BackupStruct& old_state) {
  /* Calculate place of stone */
  old_state.game_has_finished = game_has_finished;
  old_state.cur_player = cur_player;
  old_state.opp_player = opp_player;
  old_state.setting_phase = setting_phase;
  old_state.stone_part_of_mill = stone_part_of_mill;
  old_state.field = field;

  /* Check if move is possible */
  if (game_has_finished) {
    return false;
  }
  if (move.from > size) {
    return false;
  }
  if (move.to > size) {
    return false;
  }
  if (move.remove_stone > size) {
    return false;
  }

  /* Move */
  bool move_result = false;
  if (setting_phase) {
    move_result = SetStone(move, old_state);
  } else {
    move_result = NormalMove(move, old_state);
  }
  if (!move_result) {
    return false;
  }

  /* When opponent is unable to move than current player has won */
  if ((!opp_player.num_possible_moves) && (!setting_phase) &&
      (opp_player.num_stones > 3)) {
    game_has_finished = true;
  }

  /* Set next player */
  std::swap(cur_player, opp_player);

  /* Update has_only_mills */
  CalcHasOnlyMills();

  return true;
}

/* Reverts to an old state */
bool FieldStructForward::Undo(const BackupStruct& old_state) {
  game_has_finished = old_state.game_has_finished;
  cur_player = old_state.cur_player;
  opp_player = old_state.opp_player;
  setting_phase = old_state.setting_phase;
  field = old_state.field;
  stone_part_of_mill = old_state.stone_part_of_mill;
  return true;
}

}  // namespace muehle
