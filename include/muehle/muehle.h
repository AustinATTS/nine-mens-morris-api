#ifndef MUEHLE_MUEHLE_H_
#define MUEHLE_MUEHLE_H_

#include <cstdio>
#include <cstdlib>

#include "muehle/field_struct.h"
#include "muehle/muehle_ai.h"

namespace muehle {

/* Class representing the game */
class Muehle {
  /* typedef */
  using FieldPos = FieldStruct::FieldPos;
  using FieldArray = FieldStruct::FieldArray;

 public:
  struct LogItem {
    MoveInfo move = MoveInfo{};                 /* Move which is done */
    PlayerId player = PlayerId::square_is_free; /* Player who made the move */

    LogItem() = default;
    LogItem(const MoveInfo& move, PlayerId player);

    static unsigned int GetNumNormalMovesWithoutRemoveal(
        const std::vector<LogItem>& log);
    static float GetNumRepeatedMoves(const std::vector<LogItem>& log);
    bool operator==(const LogItem& other) const;
  };

  /* Constants */
  static const unsigned int max_num_moves =
      10000; /* Maximum number of moves which can be saved in the history */
  static const unsigned int num_repeated_moves_to_remis =
      3; /* Number of repeated moves after which the game is remis */

  /* Constructor / destructor */
  Muehle();
  ~Muehle();

  /* Functions */
  void BeginNewGame(MuehleAI* first_player_ai, MuehleAI* second_player_ai,
                    PlayerId current_player, bool setting_phase,
                    bool reset_field);
  void SetAI(PlayerId player, MuehleAI* ai);
  bool MoveStone(const MoveInfo& move);
  void SetNumMovesToRemis(unsigned int num_moves);
  bool RedoLastMove();
  bool UndoLastMove();

  /* Printing */
  void PrintField() const;

  /* Start the game with a customised state */
  bool SetCurrentGameState(FieldStruct& cur_state);
  bool PutStone(FieldPos pos, PlayerId player);
  bool SettingPhaseHasFinished();
  void CalcNumberOfRestingStones(int& num_white_stones_resting,
                                 int& num_black_stones_resting);

  /* Get computer choice */
  void GetComputersChoice(MoveInfo& move) const;
  void GetChoiceOfSpecialAI(MuehleAI* ai, MoveInfo& move) const;

  /* Getter */
  void GetLog(std::vector<LogItem>& log, unsigned int& current_index) const;
  const FieldArray& GetField() const;
  bool WouldMillBeClosed(const MoveInfo& move) const;
  bool IsCurrentPlayerHuman() const;
  bool IsOpponentPlayerHuman() const;
  bool InSettingPhase() const { return field.InSettingPhase(); }
  unsigned int MustStoneBeRemoved() const { return stone_must_be_removed; }
  bool GameHasFinished() const;
  PlayerId GetWinner() const;
  PlayerId GetCurrentPlayer() const { return field.GetCurPlayer().id; }
  unsigned int GetMovesDone() const { return move_log_current_index; }
  unsigned int GetNumStonesSet() const { return field.GetNumStonesSet(); }
  PlayerId GetBeginningPlayer() const { return beginning_player; }
  unsigned int GetNumStonesOfCurPlayer() const {
    return field.GetCurPlayer().num_stones;
  }
  unsigned int GetNumStonesOfOppPlayer() const {
    return field.GetOppPlayer().num_stones;
  }
  unsigned int GetNumTurnsToRemis() const;
  float GetNumRepeatedMoves() const;
  bool IsMoveAllowed(const MoveInfo& move, bool IgnoreStoneRemoval) const;

 private:
  /* Variables */
  bool stone_must_be_removed =
      false; /* True if a mill was closed and the player must remove a stone.
                This also indicates that the move is not completed yet. */
  unsigned int move_log_current_index =
      0; /* Index pointing to the current move in the history, when the user
            already went back on some moves */
  unsigned int num_moves_to_remis = 205; /* Number of moves after which the game
              is remis. This is not the current but the initial value. */
  std::vector<LogItem>
      move_log; /* Array containing the history of moves done. */
  MuehleAI* player_one_ai = nullptr; /* Class pointer to the AI of player one */
  MuehleAI* player_two_ai = nullptr; /* Class pointer to the AI of player two */
  FieldStruct field;                 /* Current field */
  FieldStruct initial_field; /* Undo of the last move is done by setting the
                                initial field and performing all moves saved in
                                history. the initial field is not necessarily an
                                empty field. It can be any state. */
  PlayerId beginning_player =
      PlayerId::player_one; /* PlayerId of the player who makes the first move
                             */
};

}  // namespace muehle

#endif  // MUEHLE_MUEHLE_H_
