#ifndef MUEHLE_MINI_MAX_PRED_VARS_H_
#define MUEHLE_MINI_MAX_PRED_VARS_H_

namespace muehle {
namespace mini_max {
namespace retro_analysis {

/* Variables describing a preceeding state */
struct PredVars {
  /* State number of the preceeding state */
  unsigned int pred_state_number;

  /* Layer number of the preceeding state */
  unsigned int pred_layer_number;

  /* Symmetry operation number, which was applied to the preceeding state */
  unsigned int pred_sym_operation;

  /* True if the player to move has changed, false if it is still the same */
  bool player_to_move_changed;
};

} /* namespace retro_analysis */
} /* namespace mini_max */
} /* namespace muehle */

#endif /* MUEHLE_MINI_MAX_PRED_VARS_H_ */
