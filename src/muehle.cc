#include "muehle/muehle.h"

#include <c++/13/ctime>

namespace muehle {

/* Muehle class constructor */
Muehle::Muehle() { srand((unsigned)time(nullptr)); }

/* Muehle class destructor */
Muehle::~Muehle() {}

/* Reinitialises the Muehle object. */
void Muehle::BeginNewGame(MuehleAI* first_player_ai, MuehleAI* second_player_ai,
                          PlayerId current_player, bool setting_phase,
                          bool reset_field) {
  /* Delete history */
  move_log_current_index = 0;
  move_log.clear();

  /* Calc beginning player */
  if (current_player == PlayerId::player_one ||
      current_player == PlayerId::player_two) {
    beginning_player = current_player;
  } else {
    beginning_player =
        (rand() % 2) ? PlayerId::player_one : PlayerId::player_two;
  }

  /* Create new field */
  if (reset_field) {
    field.Reset(beginning_player);
  }
  field.SetSituation(field.GetField(), setting_phase, 0);
  if (field.GetCurPlayer().id != beginning_player) {
    field.Invert();
  }

  /* Remember initial field */
  initial_field = field;
  player_one_ai = first_player_ai;
  player_two_ai = second_player_ai;
}

/* Put a stone onto the field during the setting phase. */
bool Muehle::PutStone(FieldPos pos, PlayerId player) {
  /* Check perameters */
  if (player != PlayerId::player_one && player != PlayerId::player_two) {
    return false;
  }
  if (pos >= FieldStruct::size) {
    return false;
  }
  if (field.GetStone(pos) != PlayerId::square_is_free) {
    return false;
  }

  /* Set stone */
  FieldStruct::FieldArray the_field = field.GetField();
  the_field[pos] = player;
  field.SetSituation(the_field, field.InSettingPhase(), 0);

  /* Return success */
  return true;
}

/* This function has to be called when the setting phase has finished */
bool Muehle::SettingPhaseHasFinished() {
  /* Remember initial_field */
  initial_field = field;
  return true;
}

/* Copy the current field state into the array p_field */
const FieldStruct::FieldArray& Muehle::GetField() const {
  return field.GetField();
}

/* Checks if the given move would close a mill */
bool Muehle::WouldMillBeClosed(const MoveInfo& move) const {
  /* Check if the move is valid */
  if (!field.InSettingPhase()) {
    if (move.from > FieldStruct::size || move.to >= FieldStruct::size) {
      return false;
    }
    if (field.GetStone(move.from) != field.GetCurPlayer().id) {
      return false;
    }
  } else {
    if (move.from != FieldStruct::size || move.to >= FieldStruct::size) {
      return false;
    }
    if (field.GetStone(move.to) != PlayerId::square_is_free) {
      return false;
    }
  }

  /* Create a copy of the field */
  FieldStruct temp_field = field;
  FieldStruct::BackupStruct dummy_state;
  temp_field.Move(move, dummy_state);

  /* Check if a mill is closed */
  return temp_field.IsStonePartOfMill(move.to) > 0;
}

/* Copy the whole history of moves into the passed arrays, which must be of size
 * [MaxNumMoves] */
void Muehle::GetLog(std::vector<LogItem>& log,
                    unsigned int& current_index) const {
  log = move_log;
  current_index = move_log_current_index;
}

/* Returns true if the current player is not assigned to AI */
bool Muehle::IsCurrentPlayerHuman() const {
  if (field.GetCurPlayer().id == PlayerId::player_one) {
    return (player_one_ai == nullptr) ? true : false;
  } else {
    return (player_two_ai == nullptr) ? true : false;
  }
}

/* Returns true if the opponent player is not assigned to AI */
bool Muehle::IsOpponentPlayerHuman() const {
  if (field.GetCurPlayer().id == PlayerId::player_one) {
    return (player_one_ai == nullptr) ? true : false;
  } else {
    return (player_two_ai == nullptr) ? true : false;
  }
}

/* Checks if the game has finished */
bool Muehle::GameHasFinished() const {
  if (GetNumTurnsToRemis() == 0) {
    return true;
  }
  if (GetNumRepeatedMoves() >= num_repeated_moves_to_remis) {
    return true;
  }
  return field.GetWinner() != PlayerId::square_is_free;
}

/* Returns PlayerId::square_is_free if no player has won yet or game drawn,
 * otherwise the PlayerId of the winner */
PlayerId Muehle::GetWinner() const {
  if (GameHasFinished()) {
    return field.GetWinner();
  }
  return PlayerId::square_is_free;
}

/* Returns the number of turns to remis */
unsigned int Muehle::GetNumTurnsToRemis() const {
  /* Only consider moves up to the current index */
  std::vector<LogItem> log_subset(move_log.begin(),
                                  move_log.begin() + move_log_current_index);
  unsigned int num_normal_moves_done =
      LogItem::GetNumNormalMovesWithoutRemoval(log_subset);
  return num_moves_to_remis - num_normal_moves_done;
}

/* Returns the number of repeated moves */
float Muehle::GetNumRepeatedMoves() const {
  /* If we are in the setting phase, we cannot have repeated moves */
  if (InSettingPhase()) {
    return 0.0f;
  }

  /* Only consider moves up to the current index */
  std::vector<LogItem> log_subset(move_log.begin(),
                                  move_log.begin() + move_log_current_index);
  return LogItem::GetNumRepeatedMoves(log_subset);
}

/* Checks if the move is allowed */
bool Muehle::IsMoveAllowed(const MoveInfo& move,
                           bool ignore_stone_removal) const {
  std::vector<unsigned int> possibility_ids;
  field.GetPossibilities(possibility_ids);
  for (const auto& id : possibility_ids) {
    if (ignore_stone_removal) {
      MoveInfo allowed_move;
      allowed_move.SetId(id);
      if (move.from == allowed_move.from && move.to == allowed_move.to) {
        return true;
      }
    } else if (move.GetId() == id) {
      return true;
    }
  }
  return false;
}

/* Assigns an AI to a player */
void Muehle::SetAI(PlayerId player, MuehleAI* ai) {
  if (player == PlayerId::player_one) {
    player_one_ai = ai;
  }
  if (player == PlayerId::player_two) {
    player_two_ai = ai;
  }
}

/* Returns the move the passed AI would do. */
void Muehle::GetChoiceOfSpecialAI(MuehleAI* ai, MoveInfo& move) const {
  FieldStruct the_field;
  move = MoveInfo{};
  the_field = field;
  if (ai != nullptr && !GameHasFinished()) {
    ai->Play(the_field, move);
  }
}

/* Returns the move the AI of the current player would do. */
void Muehle::GetComputersChoice(MoveInfo& move) const {
  FieldStruct the_field;
  move = MoveInfo{};
  the_field = field;

  if (!GameHasFinished()) {
    if (field.GetCurPlayer().id == PlayerId::player_one) {
      if (player_one_ai != nullptr) {
        player_one_ai->Play(the_field, move);
      }
    } else {
      if (player_two_ai != nullptr) {
        player_two_ai->Play(the_field, move);
      }
    }
  }
}

/* Moves a stone */
bool Muehle::MoveStone(const MoveInfo& move) {
  /* Avoid index override */
  if (GetMovesDone() >= max_num_moves) {
    return false;
  }

  /* Is the game still running? */
  if (GameHasFinished()) {
    return false;
  }

  /* Locals */
  FieldStruct::BackupStruct old_state;
  PlayerId moving_player = field.GetCurPlayer().id;
  bool is_move_completed = false;
  
  /* If a is a mill closed and no information on which stone to remove is given,
  then the move is not completed yet. */
  if (move.remove_stone == FieldStruct::size) {
    if (!stone_must_be_removed) {
      stone_must_be_removed = WouldMillBeClosed(move);
      is_move_completed = !stone_must_be_removed;
    } else {
      is_move_completed = true;
    }
    if (!is_move_completed) {
      return false;
    }
  }
  stone_must_be_removed = false;

  /* Perform move */
  if (!field.Move(move, old_state)) {
    return false;
  }

  /* Store history */
  if (move_log_current_index < move_log.size()) {
    move_log[move_log_current_index] = LogItem{move, moving_player};
    move_log.resize(move_log_current_index + 1);
  } else {
    move_log.push_back(LogItem{move, moving_player});
  }
  move_log_current_index++;

  /* Everything is ok */
  return true;
}

/* Sets the number of moves after which the game is remis. */
void Muehle::SetNumMovesToRemis(unsigned int num_moves) {
  num_moves_to_remis = num_moves ? num_moves : 10000;
}

/* Set an arbitary game state as the current one. */
bool Muehle::SetCurrentGameState(FieldStruct& cur_state) {
  field = cur_state;
  move_log_current_index = 0;
  move_log.clear();
  return true;
}

/* Calls the PrintField() function of the current field. Prints the current game
 * state on the screen. */
void Muehle::PrintField() const { field.Print(); }

/* Redo the last move */
bool Muehle::RedoLastMove(void) {
  if (move_log_current_index >= move_log.size()) {
    return false; /* No more moves to redo */
  }
  std::vector<LogItem> move_log_bak = move_log;

  /* Get last move */
  LogItem last_move = move_log[move_log_current_index];

  /* Perform move */
  MoveStone(last_move.move);

  /* Restore the log */
  move_log = move_log_bak;
  return true;
}

/* Sets the initial field as the current one and apply all (minus one) moves
 * from the move history */
bool Muehle::UndoLastMove(void) {
  /* Locals */
  unsigned int moves_done_bak = GetMovesDone();
  std::vector<LogItem> move_log_bak;

  /* At least one move must be done */
  if (!moves_done_bak) {
    return false;
  }

  /* Make backup of log */
  move_log_bak = move_log;

  /* Reset field */
  field = initial_field;
  move_log_current_index = 0;

  /* And play again, except the last move */
  for (unsigned int i = 0; i < moves_done_bak - 1; i++) {
    MoveStone(move_log_bak[i].move);
  }
  /* Restore old log */
  move_log = move_log_bak;
  return true;
}

/* Calculates the number of resting stones. */
void Muehle::CalcNumberOfRestingStones(int& num_white_stones_resting,
                                       int& num_black_stones_resting) {
  if (GetCurrentPlayer() == PlayerId::player_two) {
    num_white_stones_resting = FieldStruct::num_stones_per_player -
                               field.GetCurPlayer().num_stones_missing -
                               field.GetCurPlayer().num_stones;
    num_black_stones_resting = FieldStruct::num_stones_per_player -
                               field.GetOppPlayer().num_stones_missing -
                               field.GetOppPlayer().num_stones;
  } else {
    num_white_stones_resting = FieldStruct::num_stones_per_player -
                               field.GetOppPlayer().num_stones_missing -
                               field.GetOppPlayer().num_stones;
    num_black_stones_resting = FieldStruct::num_stones_per_player -
                               field.GetCurPlayer().num_stones_missing -
                               field.GetCurPlayer().num_stones;
  }
}

/* Constructor of the LogItem class. */
Muehle::LogItem::LogItem(const MoveInfo& move, PlayerId player)
    : move(move), player(player) {}

/* Returns the number of last normal moves without any stone removal */
unsigned int Muehle::LogItem::GetNumNormalMovesWithoutRemoval(
    const std::vector<LogItem>& log) {
  if (log.empty()) {
    return 0; /* Leave if there are no moves */
  }
  if (log.size() < 2) {
    return 0; /* Leave if there cant be normal moves */
  }

  /* Locals */
  unsigned int num_normal_moves_without_removal = 0;

  /* Iterate backwards through the log to count normal moves */
  for (auto it = log.rbegin(); it != log.rend(); ++it) {
    /* If we encounter a move that is not normal, we stop counting */
    if (it->move.IsSettingPhase()) {
      break;
    }
    /* Count the normal move */
    num_normal_moves_without_removal++;
  }

  /* Count only the moves of one player */
  num_normal_moves_without_removal /= 2;

  return num_normal_moves_without_removal;
}

/* Returns the number of repeated moves in the log.
 * A repeated move is a move where the player moves a stone back to the position
 * it was before. This function counts the number of such moves. It does not
 * count setting or removing stones. */
float Muehle::LogItem::GetNumRepeatedMoves(const std::vector<LogItem>& log) {
  /* Check if the log is valid for counting repeated moves */
  if (log.empty()) {
    return 0.0f; /* Leave if there are no moves */
  }
  if (log.size() < 3) {
    return 0.0f; /* Leave if there cant be repeated moves */
  }

  /* Consider the last move as backward_move_current_player */
  LogItem backward_move_current_player = log[log.size() - 1];
  LogItem backward_move_opponent_player = log[log.size() - 2];

  /* Consider the second last move as backward_move_opponent_player */
  if (backward_move_current_player.move.IsSettingPhase()) {
    return 0.0f; /* Leave if the last move is not a normal move */
  }
  if (backward_move_opponent_player.move.IsSettingPhase()) {
    return 0.0f; /* Leave if the second last move is not a normal move */
  }

  /* Create forward moves for the current and opponent player */
  LogItem forward_move_current_player = LogItem{
      backward_move_current_player.move, backward_move_current_player.player};
  LogItem forward_move_opponent_player = LogItem{
      backward_move_opponent_player.move, backward_move_opponent_player.player};

  /* Iterate backwards through the log to count repeated moves */
  float repeated_moves = 0.0f;
  std::vector<LogItem>::const_iterator it;
  for (it = log.end() - 1; it != log.begin(); --it) {
    /* Skip the last two moves, which are already considered */
    if (it == log.end() - 1 || it == log.end() - 2) {
      continue;
    }

    /* If not a normal move, break the loop. This is to ensure we only count
     * normal moves for repeated moves */
    if (it->move.IsSettingPhase()) {
      break;
    }

    /* Check the type of the move and count repeated moves accordingly */
    if (*it == backward_move_current_player) {
      continue;
    } else if (*it == backward_move_opponent_player) {
      continue;
    } else if (*it == forward_move_current_player) {
      repeated_moves += 0.5f; /* Count as half repeated move */
    } else if (*it == forward_move_opponent_player) {
      repeated_moves += 0.5f; /* count as half repeated move */
      /* If the move is neither a forward or backward move of the current player
       * nor the opponent player, break the loop. */
    } else {
      break;
    }
  }
  return repeated_moves;
}

/* Compares two LogItem objects for equality. */
bool Muehle::LogItem::operator==(const LogItem& other) const {
  return (move == other.move && player == other.player);
}

}  // namespace muehle
