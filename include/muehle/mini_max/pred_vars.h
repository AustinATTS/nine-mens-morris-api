#ifndef MUEHLE_MINI_MAX_PRED_VARS_H_
#define MUEHLE_MINI_MAX_PRED_VARS_H_

namespace muehle {
namespace mini_max {
namespace retro_analysis {

/* Variables describing a preceeding state */
struct PredVars {
  unsigned int pred_state_number;  /* State number of the preceeding state */
  unsigned int pred_layer_number;  /* Layer number of the preceeding state */
  unsigned int pred_sym_operation; /* Symmetry operation number, which was
                                      applied to the preceeding state */
  bool player_to_move_changed; /* True if the player to move has changed, false
                                  if it is still the same */
};

}  // namespace retro_analysis
}  // namespace mini_max
}  // namespace muehle

#endif  // MUEHLE_MINI_MAX_PRED_VARS_H_
