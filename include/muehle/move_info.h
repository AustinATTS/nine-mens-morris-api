#ifndef MUEHLE_MOVE_INFO_H_
#define MUEHLE_MOVE_INFO_H_

namespace muehle {

/* Class representing a move */
class MoveInfo {
 public:
  /* Type representing the id of a possibility */
  using PossibilityId = unsigned int;
  /* Position of the stone which is moved */
  unsigned int from = 24;
  /* Position of the stone where it is moved to */
  unsigned int to = 24;
  /* Position of the stone which is removed, otherwise the value is 'size' */
  unsigned int remove_stone = 24;

  MoveInfo() = default;
  MoveInfo(unsigned int from, unsigned int to, unsigned int remove_stone);

  bool operator==(const MoveInfo& other) const;
  PossibilityId GetId() const;
  void SetId(PossibilityId id);
  bool IsSettingPhase() const;
  static const MoveInfo& GetMoveInfo(PossibilityId id);
};

}  /* namespace muehle */

#endif  /* MUEHLE_MOVE_INFO_H_ */
