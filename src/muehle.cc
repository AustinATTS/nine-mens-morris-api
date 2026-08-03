#include "muehle/muehle.h"

#include <c++/13/ctime>

namespace muehle {

/* Muehle class constructor */
Muehle::Muehle() { srand((unsigned)time(nullptr)); }

/* Muehle class destructor */
Muehle::~Muehle() {}

/* Reinitialises the Muehle object. */
void Muehle::BeginNewGame(MuehleAI* first_player_ai, MuehleAI* second_player_ai,
                          PlayerId current_player, bool setting_phase,
                          bool reset_field) {
  /* Delete history */
  move_log_current_index = 0;
  move_log.clear();

  /* Calc beginning player */
  if (current_player == PlayerId::player_one ||
      current_player == PlayerId::player_two) {
    beginning_player = current_player;
  } else {
    beginning_player =
        (rand() % 2) ? PlayerId::player_one : PlayerId::player_two;
  }

  /* Create new field */
  if (reset_field) {
    field.Reset(beginning_player);
  }
  field.SetSituation(field.GetField(), setting_phase, 0);
  if (field.GetCurPlayer().id != beginning_player) {
    field.Invert();
  }

  /* Remember initial field */
  initial_field = field;
  player_one_ai = first_player_ai;
  player_two_ai = second_player_ai;
}

/* Put a stone onto the field during the setting phase. */
bool Muehle::PutStone(FieldPos pos, PlayerId player) {
  /* Check perameters */
  if (player != PlayerId::player_one && player != PlayerId::player_two) {
    return false;
  }
  if (pos >= FieldStruct::size) {
    return false;
  }
  if (field.GetStone(pos) != PlayerId::square_is_free) {
    return false;
  }

  /* Set stone */
  FieldStruct::FieldArray the_field = field.GetField();
  the_field[pos] = player;
  field.SetSituation(the_field, field.InSettingPhase(), 0);

  /* Return success */
  return true;
}

/* This function has to be called when the setting phase has finished */
bool Muehle::SettingPhaseHasFinished() {
  /* Remember initial_field */
  initial_field = field;
  return true;
}

/* Copy the current field state into the array p_field */
const FieldStruct::FieldArray& Muehle::GetField() const {
  return field.GetField();
}

/* Checks if the given move would clos */

}  // namespace muehle
