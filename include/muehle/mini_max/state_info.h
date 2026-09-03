#ifndef MUEHLE_MINI_MAX_STATE_INFO_H_
#define MUEHLE_MINI_MAX_STATE_INFO_H_

#include <vector>

#include "muehle/mini_max/possibility_info.h"
#include "muehle/mini_max/type_def.h"

namespace muehle {
namespace mini_max {

/* Information about a state/knot */
struct StateInfo {
  /* Possible moves. Thereby the value of a choice is from the perspective of
   * the current player. */
  std::vector<PossibilityInfo> choices;

  /* Value of the knot */
  TwoBit short_value;

  /* Number of plies till win/lose */
  PlyInfoVarType ply_info;

  /* Best amount of plies to win or lose */
  unsigned int best_amount_of_plies;

  void UpdateBestAmountOfPlies();
};

} /* namespace mini_max */
} /* namespace muehle */

#endif /* MUEHLE_MINI_MAX_STATE_INFO_H_ */
