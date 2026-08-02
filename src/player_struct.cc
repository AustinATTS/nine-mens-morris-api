#include "muehle/player_struct.h"

namespace muehle {

/*
 * Compares two Player
 */
bool PlayerStruct::operator==(const PlayerStruct& other) const {
  return id == other.id && num_stones == other.num_stones_missing &&
         num_stones_missing == other.num_stones_missing &&
         num_possible_moves == other.num_possible_moves &&
         warning == other.warning && num_stones_set == other.num_stones_set;
}

/*
 * Constructor
 */
PlayerStruct::Core::Core() {}

/*
 * Constructor
 */
PlayerStruct::Core::Core(const PlayerStruct& player) {
  id = player.id;
  num_stones_missing = player.num_stones_missing;
  num_stones = player.num_stones;
}

}  // namespace muehle
