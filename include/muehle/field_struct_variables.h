#ifndef MUEHLE_FIELD_STRUCT_VARIABLES_H_
#define MUEHLE_FIELD_STRUCT_VARIABLES_H_
#include "muehle/field_struct_types.h"

namespace muehle {

/* Additional variables describing the game state */
class FieldStructVariables : public FieldStructTypes {
  friend class FieldStruct;

 public:
  /* functions */
  bool SetSituation(const Core& core);
  bool SetSituation(const FieldArray& field, bool setting_phase,
                    unsigned int total_num_stones_missing);
  void Reset(PlayerId first_player = PlayerId::player_one);
  void Invert();
  bool IsIntegrityOk() const;
  void Print() const;

  /* getter */
  PlayerId GetWinner() const;
  bool HasGameFinished() const;
  bool InSettingPhase() const;
  unsigned int GetNumStonesSet() const;
  const PlayerStruct& GetCurPlayer() const;
  const PlayerStruct& GetOppPlayer() const;
  const FieldArray& GetField() const;
  PlayerId GetStone(FieldPos pos) const;
  unsigned int IsStonePartOfMill(FieldPos pos) const;

 protected:
  /* More constants */
  static const Array2d<FieldPos, size, 4>
      connected_square; /* Array containing the index of the neighbour or 'size'
                         */
  static const Array3d<FieldPos, size, 2, 2>
      neighbour; /* Array containing the two neighbours of each squares */

  /* Core variables */
  FieldArray field; /* One of the values above for each position, initialised
                       with square_is_free */
  bool setting_phase = true; /* True if stones_set < 18 */

  /* Deduced variables */
  MillArray
      stone_part_of_mill;  /* The number of mills this stone is a part of */
  PlayerStruct cur_player; /* Pointers to the current player */
  PlayerStruct opp_player; /* Pointers to the opponent player */
  bool game_has_finished = false; /* Someone has won or current field is full */

  /* Helper functions */
  char GetCharFromStone(PlayerId stone) const;
  static void SetConnection(Array2d<FieldPos, size, 4>& connected_square,
                            FieldPos index, int first_direction,
                            int second_direction, int third_direction,
                            int fourth_direction);
  static void SetNeighbour(Array3d<FieldPos, size, 2, 2>& neighbour,
                           FieldPos index, FieldPos first_neighbour0,
                           FieldPos second_neigbour0, FieldPos first_neighbour1,
                           FieldPos second_neigbour1);
  void SetStonePartOfMill(FieldPos stone, FieldPos first_neighbour,
                          FieldPos second_neighbour);
  void CalcNumPossibleMoves(PlayerStruct& player) const;
  void CalcStonePartOfMill();
  void CalcHasOnlyMills();
  void CalcNumberOfMills();
  void CalcNumStones();
  void CalcNumStonesSet(unsigned int total_num_stones_missing);
};
}  // namespace muehle

#endif  // MUEHLE_FIELD_STRUCT_VARIABLES_H_
