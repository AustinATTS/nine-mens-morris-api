#include "muehle/move_info.h"

#include <stdexcept>

#include "muehle/field_struct.h"

namespace muehle {

/* Constructor */
MoveInfo::MoveInfo(unsigned int from, unsigned int to,
                   unsigned int remove_stone)
    : from(from), to(to), remove_stone(remove_stone) {}

/* Compares two MoveInfos */
bool MoveInfo::operator==(const MoveInfo& other) const {
  return from == other.from && to == other.to &&
         remove_stone == other.remove_stone;
}

/* Returns the id of the MoveInfo */
MoveInfo::PossibilityId MoveInfo::GetId() const {
  return (from * (FieldStruct::size + 1) + to) * (FieldStruct::size + 1) +
         remove_stone;
}

/* Sets the id of the MoveInfo */
void MoveInfo::SetId(PossibilityId id) {
  /* Perform consistency checks */
  if (id >= ((FieldStruct::size + 1) * (FieldStruct::size + 1) *
             (FieldStruct::size + 1))) {
    throw std::out_of_range(
        "Invalid PossibilityId: id exceeds the maximum allowed value.");
  }
  from = id / ((FieldStruct::size + 1) * (FieldStruct::size + 1));
  to = (id / (FieldStruct::size + 1)) % (FieldStruct::size + 1);
  remove_stone = id % (FieldStruct::size + 1);
}

/* Returns true if the MoveInfo is in setting phase */
bool MoveInfo::IsSettingPhase() const {
  return from == FieldStruct::size && to < FieldStruct::size;
}

/* Returns the MoveInfo for a given id */
const MoveInfo& MoveInfo::GetMoveInfo(PossibilityId id) {
  static MoveInfo move;
  move.SetId(id);
  return move;
}

} /* namespace muehle */
