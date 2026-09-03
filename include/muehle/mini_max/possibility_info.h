#ifndef MUEHLE_MINI_MAX_POSSIBILITY_INFO_H_
#define MUEHLE_MINI_MAX_POSSIBILITY_INFO_H_

#include "muehle/mini_max/type_def.h"

namespace muehle {
namespace mini_max {

/* Information about a possible move */
struct PossibilityInfo {
  /* Id of the possibility returned by GameInterface::GetPossibilities() */
  unsigned int possibility_id;

  /* Value of the knot after the move, from the perspective of the current
   * player */
  TwoBit short_value;

  /* Number of plies to win or lose */
  PlyInfoVarType ply_info;

  /* Number of branches leading to a state with a certain value, from the
   * perspective of the current player */
  unsigned int freq_values_sub_moves[SKV_NUM_VALUES];
};

} /* namespace mini_max */
} /* namespace muehle */

#endif /* MUEHLE_MINI_MAX_POSSIBILITY_INFO_H_ */
