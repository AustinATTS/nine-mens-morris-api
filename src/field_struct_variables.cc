#include "muehle/field_struct_variables.h"

#include <iostream>

namespace muehle {
using namespace std;

/* Prints the field to the console */
void FieldStructVariables::Print() const {
  /* Locals */
  std::array<char, size> c;
  const bool long_version = false;

  for (FieldPos index = 0; index < size; index++) {
    c[index] = GetCharFromStone(field[index]);
  }

  cout << "current player          : " << GetCharFromStone(cur_player.id)
       << " has " << cur_player.num_stones << " stones (set "
       << cur_player.num_stones_set << ")\n";
  cout << "opponent player         : " << GetCharFromStone(opp_player.id)
       << " has " << opp_player.num_stones << " stones (set "
       << opp_player.num_stones_set << ")\n";
  cout << "setting phase           : " << (setting_phase ? "true" : "false");
  if (long_version) {
    cout << "\n";
    cout << "\n   a-----b-----c   " << c[0] << "-----" << c[1] << "-----"
         << c[2];
    cout << "\n   |     |     |   " << "|     |     |";
    cout << "\n   | d---e---f |   " << "| " << c[3] << "---" << c[4] << "---"
         << c[5] << " |";
    cout << "\n   | |   |   | |   " << "| |   |   | |";
    cout << "\n   | | g-h-i | |   " << "| | " << c[6] << "-" << c[7] << "-"
         << c[8] << " | |";
    cout << "\n   | | | | | | |   " << "| | |   | | |";
    cout << "\n   j-k-l   m-n-o   " << c[9] << "-" << c[10] << "-" << c[11]
         << "   " << c[12] << "-" << c[13] << "-" << c[14];
    cout << "\n   | | | | | | |   " << "| | |   | | |";
    cout << "\n   | | p-q-r | |   " << "| | " << c[15] << "-" << c[16] << "-"
         << c[17] << " | |";
    cout << "\n   | |   |   | |   " << "| |   |   | |";
    cout << "\n   | s---t---u |   " << "| " << c[18] << "---" << c[19] << "---"
         << c[20] << " |";
    cout << "\n   |     |     |   " << "|     |     |";
    cout << "\n   v-----w-----x   " << c[21] << "-----" << c[22] << "-----"
         << c[23];
    cout << "\n" << endl;
  } else {
    cout << "\n" << c[0] << "-----" << c[1] << "-----" << c[2];
    cout << "\n" << "| " << c[3] << "---" << c[4] << "---" << c[5] << " |";
    cout << "\n" << "| | " << c[6] << "-" << c[7] << "-" << c[8] << " | |";
    cout << "\n"
         << c[9] << "-" << c[10] << "-" << c[11] << "   " << c[12] << "-"
         << c[13] << "-" << c[14];
    cout << "\n" << "| | " << c[15] << "-" << c[16] << "-" << c[17] << " | |";
    cout << "\n" << "| " << c[18] << "---" << c[19] << "---" << c[20] << " |";
    cout << "\n" << c[21] << "-----" << c[22] << "-----" << c[23];
    cout << "\n" << endl;
  }
}

/* Resets the field to the initial state, including both players' warnings and
 * all relevant state. This function ensures that both players' warnings, stone
 * counts, mills and the board state are fully reset. */
void FieldStructVariables::Reset(PlayerId first_player) {
  game_has_finished = false;
  cur_player.id = first_player;
  setting_phase = true;
  cur_player.warning = (cur_player.id == PlayerId::player_one)
                           ? WarningId::player_one_warning
                           : WarningId::player_two_warning;
  opp_player.id = (cur_player.id == PlayerId::player_one)
                      ? PlayerId::player_two
                      : PlayerId::player_one;
  opp_player.warning = (cur_player.id == PlayerId::player_one)
                           ? WarningId::player_two_warning
                           : WarningId::player_one_warning;
  cur_player.num_stones = 0;
  opp_player.num_stones = 0;
  cur_player.num_possible_moves = 0;
  opp_player.num_possible_moves = 0;
  cur_player.num_stones_missing = 0;
  opp_player.num_stones_missing = 0;
  cur_player.number_of_mills = 0;
  opp_player.number_of_mills = 0;
  cur_player.num_stones_set = 0;
  opp_player.num_stones_set = 0;
  cur_player.has_only_mills = false;
  opp_player.has_only_mills = false;

  field.fill(PlayerId::square_is_free);
  stone_part_of_mill.fill(0);
}

/* Switches the players and inverts the field */
void FieldStructVariables::Invert() {
  std::swap(cur_player, opp_player);

  for (FieldPos k = 0; k < size; k++) {
    switch (field[k]) {
      case PlayerId::player_one:
        field[k] = PlayerId::player_two;
        break;
      case PlayerId::player_two:
        field[k] = PlayerId::player_one;
        break;
      case PlayerId::player_one_warning:
        field[k] = PlayerId::player_two_warning;
        break;
      case PlayerId::player_two_warning:
        field[k] = PlayerId::player_one_warning;
        break;
    }
  }
}

/* Returns the char representation of a stone */
char FieldStructVariables::GetCharFromStone(PlayerId stone) const {
  switch (stone) {
    case PlayerId::player_one:
      return 'x';
    case PlayerId::player_two:
      return 'o';
    case PlayerId::player_one_warning:
      return '1';
    case PlayerId::player_two_warning:
      return '2';
    case PlayerId::player_both_warning:
      return '3';
    case PlayerId::square_is_free:
      return ' ';
  }
  return 'f';
}

/* Helper function to set 'connection_square' */
void FieldStructVariables::SetConnection(
    Array2d<FieldPos, size, 4>& connected_square, FieldPos index,
    int first_direction, int second_direction, int third_direction,
    int fourth_direction) {
  connected_square[index][0] = first_direction;
  connected_square[index][1] = second_direction;
  connected_square[index][2] = third_direction;
  connected_square[index][3] = fourth_direction;
}

/* Helper function to set 'neighbour' */
void FieldStructVariables::SetNeighbour(
    Array3d<FieldPos, size, 2, 2>& neighbour, FieldPos index,
    FieldPos first_neighbour0, FieldPos second_neigbour0,
    FieldPos first_neighbour1, FieldPos second_neigbour1) {
  neighbour[index][0][0] = first_neighbour0;
  neighbour[index][0][1] = second_neigbour0;
  neighbour[index][1][0] = first_neighbour1;
  neighbour[index][1][1] = second_neigbour1;
}

/* Returns the winner of the game, and PlayerId::square_is_free if the game is
 * not finished. */
PlayerId FieldStructVariables::GetWinner() const {
  PlayerId winner = PlayerId::square_is_free;

  if ((!cur_player.num_possible_moves) & (!setting_phase) &&
      (cur_player.num_stones > 3)) {
    winner = opp_player.id;
  }
  if ((cur_player.num_stones < 3) && (!setting_phase)) {
    winner = opp_player.id;
  }
  if ((opp_player.num_stones < 3) && (!setting_phase)) {
    winner = cur_player.id;
  }

  return winner;
}

/* Returns a reference to the current player */
const PlayerStruct& FieldStructVariables::GetCurPlayer() const {
  return cur_player;
}

/* Returns a reference to the opponent player */
const PlayerStruct& FieldStructVariables::GetOppPlayer() const {
  return opp_player;
}

/* Returns the player id of a stone */
PlayerId FieldStructVariables::GetStone(FieldPos pos) const {
  return field[pos];
}

/* Returns the number of mills, of which this stone is part of */
unsigned int FieldStructVariables::IsStonePartOfMill(FieldPos pos) const {
  return stone_part_of_mill[pos];
}

/* Returns the field */
const FieldStructVariables::FieldArray& FieldStructVariables::GetField() const {
  return field;
}

/* Returns true if the game has finished */
bool FieldStructVariables::HasGameFinished() const { return game_has_finished; }

/* Returns the number of stones set in the setting phase */
unsigned int FieldStructVariables::GetNumStonesSet() const {
  return cur_player.num_stones_set + opp_player.num_stones_set;
}

/* Returns true if the game is in setting phase */
bool FieldStructVariables::InSettingPhase() const { return setting_phase; }

/* Updates 'has_only_mills'. stone_part_of_mill and field must be in sync */
void FieldStructVariables::CalcHasOnlyMills() {
  /* Update each player */
  for (PlayerStruct* player : {&opp_player, &cur_player}) {
    /* If player has no mills then he cannot have only mills */
    if (!player->number_of_mills) {
      player->has_only_mills = false;
      continue;
    }

    player->has_only_mills = true;
    for (FieldPos i = 0; i < size; i++) {
      if (field[i] == player->id && !stone_part_of_mill[i]) {
        player->has_only_mills = false;
        break;
      }
    }
  }
}

/* Updates the number of mills for each player */
void FieldStructVariables::CalcNumberOfMills() {
  /* Count completed mills */
  cur_player.number_of_mills = 0;
  opp_player.number_of_mills = 0;
  for (FieldPos i = 0; i < size; i++) {
    if (field[i] == cur_player.id) {
      cur_player.number_of_mills += stone_part_of_mill[i];
    } else {
      opp_player.number_of_mills += stone_part_of_mill[i];
    }
  }
  cur_player.number_of_mills /= 3;
  opp_player.number_of_mills /= 3;
}

/* Updates the number of stones for each player */
void FieldStructVariables::CalcNumStones() {
  /* Count stones */
  cur_player.num_stones = 0;
  opp_player.num_stones = 0;
  for (FieldPos i = 0; i < size; i++) {
    if (field[i] == PlayerId::square_is_free) {
      continue;
    }
    if (field[i] == cur_player.id) {
      cur_player.num_stones++;
    } else {
      opp_player.num_stones++;
    }
  }
}

/* Updates the number of stones set for each player */
void FieldStructVariables::CalcNumStonesSet(
    unsigned int total_num_stones_missing) {
  unsigned int total_num_stones_set =
      cur_player.num_stones + opp_player.num_stones + total_num_stones_missing;
  cur_player.num_stones_set = total_num_stones_set / 2;
  opp_player.num_stones_set =
      total_num_stones_set / 2 + total_num_stones_set % 2;
}

/* Updates the number of possible moves for a player. Thereby the possibilities
 * containing a stone removal are not counted. */
void FieldStructVariables::CalcNumPossibleMoves(PlayerStruct& player) const {
  /* Locals */
  FieldPos i, j, k;
  unsigned int moving_direction;

  /* Setting phase */
  if (setting_phase) {
    player.num_possible_moves =
        size - cur_player.num_stones - opp_player.num_stones;
    return;
  }

  /* Normal phase */
  player.num_possible_moves = 0;

  /* Only adjacent moves allowed */
  if (player.num_stones > 3) {
    /* Optimise by avoiding repeated bounds checks and unnecessary variable
     * assignments */
    for (FieldPos i = 0; i < size; ++i) {
      if (field[i] != player.id) {
        continue;
      }
      const auto& connections = connected_square[i];
      for (unsigned int dir = 0; dir < 4; ++dir) {
        FieldPos j = connections[dir];
        /* Only increment if j is a valid position and free */
        if (j < size && field[j] == PlayerId::square_is_free) {
          ++player.num_possible_moves;
        }
      }
    }
    /* Jumping allowed: any free position */
  } else if (player.num_stones == 3) {
    for (i = 0; i < size; ++i) {
      if (field[i] != player.id) {
        continue;
      }
      for (j = 0; j < size; ++j) {
        if (field[j] == PlayerId::square_is_free) {
          player.num_possible_moves++;
        }
      }
    }
    /* Less than 3 stones: no moves possible */
  } else {
    player.num_possible_moves = 0;
  }
}

/* Updates the stone_part_of_mill array for each player */
void FieldStructVariables::CalcStonePartOfMill() {
  for (FieldPos i = 0; i < size; i++) {
    stone_part_of_mill[i] = 0;
  }
  for (FieldPos i = 0; i < size; i++) {
    SetStonePartOfMill(i, neighbour[i][0][0], neighbour[i][0][1]);
    SetStonePartOfMill(i, neighbour[i][1][0], neighbour[i][1][1]);
  }
  /* Since every mill would be detected 3 times */
  for (FieldPos i = 0; i < size; i++) {
    stone_part_of_mill[i] /= 3;
  }
}

/* Sets the stone_part_of_mill array */
void FieldStructVariables::SetStonePartOfMill(FieldPos stone,
                                              FieldPos first_neighbour,
                                              FieldPos second_neighbour) {
  /* Locals */
  PlayerId row_owner = field[stone];

  /* Mill closed? */
  if (row_owner != PlayerId::square_is_free &&
      field[first_neighbour] == row_owner &&
      field[second_neighbour] == row_owner) {
    stone_part_of_mill[stone]++;
    stone_part_of_mill[first_neighbour]++;
    stone_part_of_mill[second_neighbour]++;
  }
}

/* Sets the field to a specific state. */
bool FieldStructVariables::SetSituation(const FieldArray& field,
                                        bool setting_phase,
                                        unsigned int total_num_stones_missing) {
  /* Check for too many stones missing */
  if (total_num_stones_missing > 2 * num_stones_per_player) {
    return false;
  }

  /* total_num_stones_missing is not used during moves phase */
  if (!setting_phase && total_num_stones_missing) {
    return false;
  }

  /* Copy */
  this->field = field;
  this->setting_phase = setting_phase;
  game_has_finished = false;

  /* Set .num_stones */
  CalcNumStones();

  /* If current player already set 9 stones, then it cannot be setting phase any
   * more */
  if (setting_phase && cur_player.num_stones >= 9) {
    return false;
  }

  /* If there are too many stones missing, the situation is invalid */
  if (total_num_stones_missing > 2 * num_stones_per_player -
                                     cur_player.num_stones -
                                     opp_player.num_stones) {
    return false;
  }

  /* Set .stone_part_of_mill */
  CalcStonePartOfMill();

  /* Set .number_of_mills */
  CalcNumberOfMills();

  /* During setting phase, the total number of missing stones must be at least
   * the number of present on the field */
  if (setting_phase &&
      total_num_stones_missing <
          cur_player.number_of_mills + opp_player.number_of_mills) {
    return false;
  }

  /* stones_set and num_stones_missing */
  if (setting_phase) {
    /* The number of mills destroyed during setting phase is given from outside
     */
    CalcNumStonesSet(total_num_stones_missing);
    /* There must not be more stones on the field then set */
    if (cur_player.num_stones > cur_player.num_stones_set) {
      return false;
    }
    if (opp_player.num_stones > opp_player.num_stones_set) {
      return false;
    }
    cur_player.num_stones_missing =
        cur_player.num_stones_set - cur_player.num_stones;
    opp_player.num_stones_missing =
        opp_player.num_stones_set - opp_player.num_stones;
  } else {
    cur_player.num_stones_missing =
        num_stones_per_player - cur_player.num_stones;
    opp_player.num_stones_missing =
        num_stones_per_player - opp_player.num_stones;
    cur_player.num_stones_set = num_stones_per_player;
    opp_player.num_stones_set = num_stones_per_player;
  }

  /* If current player set 9 stones, then it cannot be setting phase any more */
  if (setting_phase &&
      cur_player.num_stones + cur_player.num_stones_missing >= 9) {
    return false;
  }

  /* Calculate number of possible moves */
  CalcNumPossibleMoves(cur_player);
  CalcNumPossibleMoves(opp_player);

  /* Update .has_only_mills for each player */
  CalcHasOnlyMills();

  /* When opponent is unable to move than current player has won */
  if (GetWinner() != PlayerId::square_is_free) {
    game_has_finished = true;
  }

  /* Test if field is ok */
  return IsIntegrityOk();
}

/* Checks if the field is in a valid state. The following member variables are
 * not verified: .field, .stone_part_of_mill, .game_has_finished,
 * .has_only_mills, .num_possible_moves, .num_stones_missing, ... */
bool FieldStructVariables::IsIntegrityOk() const {
  if (setting_phase) {
    /* If 18 stones have been set, then it cannot be the setting phase anymore
     */
    if (GetNumStonesSet() >= 18) {
      return false;
    }

    /* If current player already set 9 stones, then it cannot be setting phase
     * any more */
    if (cur_player.num_stones >= 9) {
      return false;
    }

    /* If there are too many stones are missing the situation is invalid */
    if (cur_player.num_stones_missing + opp_player.num_stones_missing >
        2 * num_stones_per_player - cur_player.num_stones -
            opp_player.num_stones) {
      return false;
    }

    /* During setting phase, the total number of missing stones must be at least
     * the number of present on the field */
    if (cur_player.num_stones_missing + opp_player.num_stones_missing <
        cur_player.number_of_mills + opp_player.number_of_mills) {
      return false;
    }

    /* If current player set 9 stones, then it cannot be setting phase any more
     */
    if (cur_player.num_stones + cur_player.num_stones_missing >= 9) {
      return false;
    }
    if (opp_player.num_stones + opp_player.num_stones_missing > 9) {
      return false;
    }

    /* Each missing stone of a player must correspond to a mill of the other
     * player on the field or a former mill, which has already been destroyed.
     * Each destroyed mill of a player requires a missing stone of that player.
     */
    if (cur_player.num_stones_missing >
        opp_player.number_of_mills + opp_player.num_stones_missing) {
      return false;
    }
    if (opp_player.num_stones_missing >
        cur_player.number_of_mills + cur_player.num_stones_missing) {
      return false;
    }

    /* If there are any stones missing, then at least one player must have mills
     */
    if (cur_player.num_stones_missing + opp_player.num_stones_missing > 0 &&
        (cur_player.number_of_mills + opp_player.number_of_mills) == 0) {
      return false;
    }

    /* If next move would be in moving phase then game must not be lost */
    if (cur_player.num_stones_set >= 8 && cur_player.num_stones < 2) {
      return false;
    }
    if (opp_player.num_stones_set >= 9 && opp_player.num_stones < 3) {
      return false;
    }

    /* Check consistency of number of stones on field with the number of mills.
     * Number of stones set might be equal, or opponent might be one stone ahead
     */
    int num_stones_set_by_cur_player =
        cur_player.num_stones + cur_player.num_stones_missing;
    int num_stones_set_by_opp_player =
        opp_player.num_stones + opp_player.num_stones_missing;
    if (!(num_stones_set_by_cur_player - num_stones_set_by_opp_player == 0 ||
          num_stones_set_by_cur_player - num_stones_set_by_opp_player == -1)) {
      return false;
    }

    /* Check consistency between variables num_stones, num_stones_missing and
     * num_stones_set */
    if (num_stones_set_by_cur_player != cur_player.num_stones_set) {
      return false;
    }
    if (num_stones_set_by_opp_player != opp_player.num_stones_set) {
      return false;
    }

    /* Moving phase */
  } else {
    /* Each player must have at least 2 stones */
    if (cur_player.num_stones < 2 || opp_player.num_stones < 2) {
      return false;
    }

    if (game_has_finished) {
      /* If game is finished then the opponent must have a mill */
      if (cur_player.num_stones < 3 && opp_player.number_of_mills == 0) {
        return false;
      }
      /* Or current player is immobilised */
      if (cur_player.num_possible_moves && cur_player.num_stones >= 3) {
        return false;
      }
    }
  }
  return true;
}

}  // namespace muehle
