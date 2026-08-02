#include "muehle/field_struct.h"

namespace muehle {

/*
 * Constructor
 */
FieldStruct::FieldStruct() { Reset(); }

/*
 * Copy Constructor
 */
FieldStruct::FieldStruct(const FieldStruct& other)
    : FieldStructForward(other), FieldStructReverse(other) {
  /* Copy the core variables from the other FieldStruct */
  this->field = other.field;
  this->setting_phase = other.setting_phase;
  this->cur_player = other.cur_player;
  this->opp_player = other.opp_player;
  this->stone_part_of_mill = other.stone_part_of_mill;
  this->game_has_finished = other.game_has_finished;
}

/*
 * Destructor
 */
FieldStruct::~FieldStruct() {}

/*
 * Compares two FieldStructs
 */
bool FieldStruct::operator==(const FieldStruct& other) const {
  return cur_player == other.cur_player && opp_player == other.opp_player &&
         setting_phase == other.setting_phase &&
         game_has_finished == other.game_has_finished && field == other.field &&
         stone_part_of_mill == other.stone_part_of_mill;
}

/*
 * Constructor
 */
FieldStruct::Core::Core() {}

/*
 * Constructor
 */
FieldStruct::Core::Core(const FieldStructVariables& vars) {
  field = vars.GetField();
  setting_phase = vars.InSettingPhase();
  cur_player = vars.GetCurPlayer();
  opp_player = vars.GetOppPlayer();
}

/* Static variables */
const FieldStruct::Array2d<FieldStruct::FieldPos, FieldStruct::size, 4>
    FieldStructVariables::connected_square = []() {
      /* Locals */
      Array2d<FieldPos, size, 4> connected_square;
      auto i = size;

      /* Set connections */
      SetConnection(connected_square, 0, 1, 9, i, i);
      SetConnection(connected_square, 1, 2, 4, 0, i);
      SetConnection(connected_square, 2, i, 14, 1, i);
      SetConnection(connected_square, 3, 4, 10, i, i);
      SetConnection(connected_square, 4, 5, 7, 3, 1);
      SetConnection(connected_square, 5, i, 13, 4, i);
      SetConnection(connected_square, 6, 7, 11, i, i);
      SetConnection(connected_square, 7, 8, i, 6, 4);
      SetConnection(connected_square, 8, i, 12, 7, i);
      SetConnection(connected_square, 9, 10, 21, i, 0);
      SetConnection(connected_square, 10, 11, 18, 9, 3);
      SetConnection(connected_square, 11, i, 15, 10, 6);
      SetConnection(connected_square, 12, 13, 17, i, 8);
      SetConnection(connected_square, 13, 14, 20, 12, 5);
      SetConnection(connected_square, 14, i, 23, 13, 2);
      SetConnection(connected_square, 15, 16, i, i, 11);
      SetConnection(connected_square, 16, 17, 19, 15, i);
      SetConnection(connected_square, 17, i, i, 16, 12);
      SetConnection(connected_square, 18, 19, i, i, 10);
      SetConnection(connected_square, 19, 20, 22, 18, 16);
      SetConnection(connected_square, 20, i, i, 19, 13);
      SetConnection(connected_square, 21, 22, i, i, 9);
      SetConnection(connected_square, 22, 23, i, 21, 19);
      SetConnection(connected_square, 23, i, i, 22, 14);

      return connected_square;
    }();

const FieldStruct::Array3d<FieldStruct::FieldPos, FieldStruct::size, 2, 2>
    FieldStructVariables::neighbour = []() {
      /* Locals */
      Array3d<FieldPos, size, 2, 2> neighbour;

      /* Neighbours */
      SetNeighbour(neighbour, 0, 1, 2, 9, 21);
      SetNeighbour(neighbour, 1, 0, 2, 4, 7);
      SetNeighbour(neighbour, 2, 0, 1, 14, 23);
      SetNeighbour(neighbour, 3, 4, 5, 10, 18);
      SetNeighbour(neighbour, 4, 1, 7, 3, 5);
      SetNeighbour(neighbour, 5, 3, 4, 13, 20);
      SetNeighbour(neighbour, 6, 7, 8, 11, 15);
      SetNeighbour(neighbour, 7, 1, 4, 6, 8);
      SetNeighbour(neighbour, 8, 6, 7, 12, 17);
      SetNeighbour(neighbour, 9, 10, 11, 0, 21);
      SetNeighbour(neighbour, 10, 9, 11, 3, 18);
      SetNeighbour(neighbour, 11, 9, 10, 6, 15);
      SetNeighbour(neighbour, 12, 13, 14, 8, 17);
      SetNeighbour(neighbour, 13, 12, 14, 5, 20);
      SetNeighbour(neighbour, 14, 12, 13, 2, 23);
      SetNeighbour(neighbour, 15, 6, 11, 16, 17);
      SetNeighbour(neighbour, 16, 15, 17, 19, 22);
      SetNeighbour(neighbour, 17, 15, 16, 8, 12);
      SetNeighbour(neighbour, 18, 3, 10, 19, 20);
      SetNeighbour(neighbour, 19, 18, 20, 16, 22);
      SetNeighbour(neighbour, 20, 5, 13, 18, 19);
      SetNeighbour(neighbour, 21, 0, 9, 22, 23);
      SetNeighbour(neighbour, 22, 16, 19, 21, 23);
      SetNeighbour(neighbour, 23, 2, 14, 21, 22);

      return neighbour;
    }();

}  // namespace muehle
