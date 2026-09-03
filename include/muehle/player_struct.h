#ifndef MUEHLE_PLAYER_STRUCT_H_
#define MUEHLE_PLAYER_STRUCT_H_

namespace muehle {

/* Enums */
enum class PlayerId : unsigned int {
  square_is_free = 0,
  player_one = 1,
  player_two = 2,
  player_one_warning = 4,
  player_two_warning = 8,
  player_both_warning = 12,
  invalid = 1000
};

enum class WarningId : unsigned int {
  no_warning = 0,
  player_one_warning = 4,
  player_two_warning = 8,
  player_both_warning = 12,
  invalid = 1000
};

/* Class representing a player */
class PlayerStruct {
 public:
  /* Constant over lifetime */
  PlayerId id;
  /* Constant over lifetime */
  WarningId warning;
  /* Number of stones of this player on the field */
  unsigned int num_stones = 0;
  /* Number of stones which has been stolen by the opponent */
  unsigned int num_stones_missing = 0;
  /* Number of stones which have been set on the field during setting phase */
  unsigned int num_stones_set = 0;
  /* Number of possible moves (setting and moving phast). Does not include
   * possible stone removals. */
  unsigned int num_possible_moves = 24;
  /* Number of mills belonging to this player */
  unsigned int number_of_mills = 0;
  /* True if the player has only mills and no non mill stones on the field */
  unsigned int has_only_mills = false;

  bool operator==(const PlayerStruct& other) const;

  /* Class containing a reduced set of variables to skip unneeded computations */
  class Core {
   public:
    PlayerId id = PlayerId::square_is_free;
    unsigned int num_stones = 0;
    unsigned int num_stones_missing = 0;

    Core();
    Core(const PlayerStruct& player);
  };
};

} /* namespace muehle */

#endif /* MUEHLE_PLAYER_STRUCT_H_ */
