#include "muehle/field_struct_types.h"

namespace muehle {

/*
 * Returns a reference to the current player
 */
const PlayerStruct::Core& FieldStructTypes::Core::GetCurPlayer() const {
  return cur_player;
}

/*
 * Returns a reference to the opponent player
 */
const PlayerStruct::Core& FieldStructTypes::Core::GetOppPlayer() const {
  return opp_player;
}

/*
 * Returns the player id of a stone
 */
PlayerId FieldStructTypes::Core::GetStone(FieldPos pos) const {
  return field[pos];
}

/*
 * Returns true if the game is in the setting phase
 */
bool FieldStructTypes::Core::InSettingPhase() const { return setting_phase; }

}  // namespace muehle
