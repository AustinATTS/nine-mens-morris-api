#include "muehle/field_struct_types.h"

namespace muehle {

const PlayerStruct::Core& FieldStructTypes::Core::GetCurPlayer() const {
  return cur_player;
}

const PlayerStruct::Core& FieldStructTypes::Core::GetOppPlayer() const {
  return opp_player;
}

PlayerId FieldStructTypes::Core::GetStone(FieldPos pos) const {
  return field[pos];
}

bool FieldStructTypes::Core::InSettingPhase() const {
  return setting_phase;
}

}  // namespace muehle
