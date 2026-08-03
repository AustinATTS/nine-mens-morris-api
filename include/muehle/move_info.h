#ifndef MUEHLE_MOVE_INFO_H_
#define MUEHLE_MOVE_INFO_H_

namespace muehle {

/* Class representing a move */
class MoveInfo {
 public:
  using PossibilityId =
      unsigned int;       /* Type representing the id of a possibility */
  unsigned int from = 24; /* Position of the stone which is moved */
  unsigned int to = 24;   /* Position of the stone where it is moved to */
  unsigned int remove_stone = 24; /* Position of the stone which is removed,
                                     otherwise the value is 'size' */

  MoveInfo() = default;
  MoveInfo(unsigned int from, unsigned int to, unsigned int remove_stone);

  bool operator==(const MoveInfo& other) const;
  PossibilityId GetId() const;
  void SetId(PossibilityId id);
  bool IsSettingPhase() const;
  static const MoveInfo& GetMoveInfo(PossibilityId id);
};

}  // namespace muehle

#endif  // MUEHLE_MOVE_INFO_H_
