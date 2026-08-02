#ifndef MUEHLE_FIELD_STRUCT_FORWARD_H_
#define MUEHLE_FIELD_STRUCT_FORWARD_H_

#include <vector>

#include "muehle/field_struct_variables.h"
#include "muehle/move_info.h"

namespace muehle {

/* Functions related to move and undo of stones */
class FieldStructForward : virtual public FieldStructVariables {
 public:
  /* Structure to save the backup of the field */
  struct BackupStruct {
    FieldArray field;
    MillArray stone_part_of_mill;
    unsigned int stones_set;
    bool setting_phase;
    bool game_has_finished;
    unsigned int stone_must_be_removed;
    PlayerStruct cur_player;
    PlayerStruct opp_player;
  };

  /* Move functions */
  bool Move(const MoveInfo& move, BackupStruct& old_state);
  bool Undo(const BackupStruct& old_state);

  /* Getter */
  void GetPossibilities(
      std::vector<MoveInfo::PossibilityId>& possibility_ids) const;

 private:
  /* Move functions */
  bool SetStone(const MoveInfo& move, BackupStruct& backup);
  bool NormalMove(const MoveInfo& move, BackupStruct& backup);
  bool RemoveStone(const MoveInfo& move, BackupStruct& backup);

  /* Get possibilities */
  void GetPossSettingPhase(
      std::vector<MoveInfo::PossibilityId>& possibility_ids) const;
  void GetPossNormalMove(
      std::vector<MoveInfo::PossibilityId>& possibility_ids) const;
  void GetPossStoneRemove(std::vector<FieldPos>& removable_stones) const;
  unsigned int WouldMillBeClosed(FieldPos from, FieldPos to) const;
  bool CanStoneBeRemoved(FieldPos pos) const;

  /* Helper functions */
  void UpdateWarning(FieldPos first_stone, FieldPos second_stone,
                     PlayerId acting_player);
  void UpdatePossibleMoves(FieldPos stone, PlayerStruct& stone_owner,
                           bool stone_removed, FieldPos ignore_stone);
  void UpdateStonePartOfMill(FieldPos stone_one, FieldPos stone_two,
                             FieldPos stone_three, PlayerId acting_player);
};

}  // namespace muehle

#endif  // MUEHLE_FIELD_STRUCT_FORWARD_H_
