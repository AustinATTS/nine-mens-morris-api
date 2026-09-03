#ifndef MUEHLE_FIELD_STRUCT_TYPES_H_
#define MUEHLE_FIELD_STRUCT_TYPES_H_

#include <array>

#include "muehle/player_struct.h"

namespace muehle {

/* Forward declarations */
class FieldStructVariables;

/* Class containing the types and constants */
class FieldStructTypes {
 public:
  /* Constants */

  /* 3 stones can be moved to 18 positions, 9 stones can be removed */
  static const unsigned int max_num_pos_moves = 3 * 18 * 9;

  /* Define player one as black (PlayerId::player_one is used for black stones)
   */
  static const PlayerId player_black = PlayerId::player_one;

  /* Define player two as white (PlayerId::player_two is used for white stones)
   */
  static const PlayerId player_white = PlayerId::player_two;

  /* Number of stones per player */
  static const unsigned int num_stones_per_player = 9;

  /* Number of squares */
  static const int size = 24;

  /* Only a nonzero value */
  static const int game_drawn = 3;

  /* typedef */

  /* Type representing the position of a stone on the field */
  using FieldPos = unsigned int;

  /* Type representing the field as an array of PlayerIds, indicating the stone
   * on each field position */
  using FieldArray = std::array<PlayerId, size>;

  /* Type representing the mills as an array of unsigned ints, indicating the
   * number of mills, of which this stone is part of */
  using MillArray = std::array<unsigned int, size>;

  /* Class containing a reduced set of variables to skip unneeded computations
   */
  class Core {
   public:
    /* Core variables */

    /* One of the values above for each field position, initialised with
     * square_is_free */
    FieldArray field;

    /* True if stones_set < 18 */
    bool setting_phase = true;

    /* Pointers to the current player */
    PlayerStruct::Core cur_player;

    /* Pointers to the opponent player */
    PlayerStruct::Core opp_player;

    Core();
    Core(const FieldStructVariables& vars);

    const PlayerStruct::Core& GetCurPlayer() const;
    const PlayerStruct::Core& GetOppPlayer() const;
    PlayerId GetStone(FieldPos pos) const;
    bool InSettingPhase() const;
  };

 protected:
  /* An alias template for a two dimensional std::array */
  template <typename T, std::size_t Row, std::size_t Col>
  using Array2d = std::array<std::array<T, Col>, Row>;

  /* An alias template for a three dimensional std::array */
  template <typename T, std::size_t Row, std::size_t Col, std::size_t Depth>
  using Array3d = std::array<Array2d<T, Depth, Col>, Row>;
};

} /* namespace muehle */

#endif /* MUEHLE_FIELD_STRUCT_TYPES_H_ */
