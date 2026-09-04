#include "muehle/field_struct.h"

#include <algorithm>
#include <cassert>

namespace muehle {

/* Constructor */
FieldStruct::FieldStruct() {
  Reset();
}

/* Copy Constructor */
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

/* Destructor */
FieldStruct::~FieldStruct() {}

/* Compares two FieldStructs */
bool FieldStruct::operator==(const FieldStruct& other) const {
  return cur_player == other.cur_player && opp_player == other.opp_player &&
         setting_phase == other.setting_phase &&
         game_has_finished == other.game_has_finished && field == other.field &&
         stone_part_of_mill == other.stone_part_of_mill;
}

/* Constructor */
FieldStruct::Core::Core() {}

/* Constructor */
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
      SetConnection(connected_square, /* index: */ 0, /* first_direction: */ 1,
                    /* second_direction: */ 9, /* third_direction: */ i,
                    /* fourth_direction: */ i);
      SetConnection(connected_square, /* index: */ 1, /* first_direction: */ 2,
                    /* second_direction: */ 4, /* third_direction: */ 0,
                    /* fourth_direction: */ i);
      SetConnection(connected_square, /* index: */ 2, /* first_direction: */ i,
                    /* second_direction: */ 14, /* third_direction: */ 1,
                    /* fourth_direction: */ i);
      SetConnection(connected_square, /* index: */ 3, /* first_direction: */ 4,
                    /* second_direction: */ 10, /* third_direction: */ i,
                    /* fourth_direction: */ i);
      SetConnection(connected_square, /* index: */ 4, /* first_direction: */ 5,
                    /* second_direction: */ 7, /* third_direction: */ 3,
                    /* fourth_direction: */ 1);
      SetConnection(connected_square, /* index: */ 5, /* first_direction: */ i,
                    /* second_direction: */ 13, /* third_direction: */ 4,
                    /* fourth_direction: */ i);
      SetConnection(connected_square, /* index: */ 6, /* first_direction: */ 7,
                    /* second_direction: */ 11, /* third_direction: */ i,
                    /* fourth_direction: */ i);
      SetConnection(connected_square, /* index: */ 7, /* first_direction: */ 8,
                    /* second_direction: */ i, /* third_direction: */ 6,
                    /* fourth_direction: */ 4);
      SetConnection(connected_square, /* index: */ 8, /* first_direction: */ i,
                    /* second_direction: */ 12, /* third_direction: */ 7,
                    /* fourth_direction: */ i);
      SetConnection(connected_square, /* index: */ 9, /* first_direction: */ 10,
                    /* second_direction: */ 21, /* third_direction: */ i,
                    /* fourth_direction: */ 0);
      SetConnection(connected_square, /* index: */ 10,
                    /* first_direction: */ 11, /* second_direction: */ 18,
                    /* third_direction: */ 9, /* fourth_direction: */ 3);
      SetConnection(connected_square, /* index: */ 11, /* first_direction: */ i,
                    /* second_direction: */ 15, /* third_direction: */ 10,
                    /* fourth_direction: */ 6);
      SetConnection(connected_square, /* index: */ 12,
                    /* first_direction: */ 13, /* second_direction: */ 17,
                    /* third_direction: */ i, /* fourth_direction: */ 8);
      SetConnection(connected_square, /* index: */ 13,
                    /* first_direction: */ 14, /* second_direction: */ 20,
                    /* third_direction: */ 12, /* fourth_direction: */ 5);
      SetConnection(connected_square, /* index: */ 14, /* first_direction: */ i,
                    /* second_direction: */ 23, /* third_direction: */ 13,
                    /* fourth_direction: */ 2);
      SetConnection(connected_square, /* index: */ 15,
                    /* first_direction: */ 16, /* second_direction: */ i,
                    /* third_direction: */ i, /* fourth_direction: */ 11);
      SetConnection(connected_square, /* index: */ 16,
                    /* first_direction: */ 17, /* second_direction: */ 19,
                    /* third_direction: */ 15, /* fourth_direction: */ i);
      SetConnection(connected_square, /* index: */ 17, /* first_direction: */ i,
                    /* second_direction: */ i, /* third_direction: */ 16,
                    /* fourth_direction: */ 12);
      SetConnection(connected_square, /* index: */ 18,
                    /* first_direction: */ 19, /* second_direction: */ i,
                    /* third_direction: */ i, /* fourth_direction: */ 10);
      SetConnection(connected_square, /* index: */ 19,
                    /* first_direction: */ 20, /* second_direction: */ 22,
                    /* third_direction: */ 18, /* fourth_direction: */ 16);
      SetConnection(connected_square, /* index: */ 20, /* first_direction: */ i,
                    /* second_direction: */ i, /* third_direction: */ 19,
                    /* fourth_direction: */ 13);
      SetConnection(connected_square, /* index: */ 21,
                    /* first_direction: */ 22, /* second_direction: */ i,
                    /* third_direction: */ i, /* fourth_direction: */ 9);
      SetConnection(connected_square, /* index: */ 22,
                    /* first_direction: */ 23, /* second_direction: */ i,
                    /* third_direction: */ 21, /* fourth_direction: */ 19);
      SetConnection(connected_square, /* index: */ 23, /* first_direction: */ i,
                    /* second_direction: */ i, /* third_direction: */ 22,
                    /* fourth_direction: */ 14);

      return connected_square;
    }();

const FieldStruct::Array3d<FieldStruct::FieldPos, FieldStruct::size, 2, 2>
    FieldStructVariables::neighbour = []() {
      /* Locals */
      Array3d<FieldPos, size, 2, 2> neighbour;

      /* Neighbours */
      SetNeighbour(neighbour, /* index: */ 0, /* first_neighbour0: */ 1,
                   /* second_neighbour0: */ 2, /* first_neighbour1: */ 9,
                   /* second_neighbour1: */ 21);
      SetNeighbour(neighbour, /* index: */ 1, /* first_neighbour0: */ 0,
                   /* second_neighbour0: */ 2, /* first_neighbour1: */ 4,
                   /* second_neighbour1: */ 7);
      SetNeighbour(neighbour, /* index: */ 2, /* first_neighbour0: */ 0,
                   /* second_neighbour0: */ 1, /* first_neighbour1: */ 14,
                   /* second_neighbour1: */ 23);
      SetNeighbour(neighbour, /* index: */ 3, /* first_neighbour0: */ 4,
                   /* second_neighbour0: */ 5, /* first_neighbour1: */ 10,
                   /* second_neighbour1: */ 18);
      SetNeighbour(neighbour, /* index: */ 4, /* first_neighbour0: */ 1,
                   /* second_neighbour0: */ 7, /* first_neighbour1: */ 3,
                   /* second_neighbour1: */ 5);
      SetNeighbour(neighbour, /* index: */ 5, /* first_neighbour0: */ 3,
                   /* second_neighbour0: */ 4, /* first_neighbour1: */ 13,
                   /* second_neighbour1: */ 20);
      SetNeighbour(neighbour, /* index: */ 6, /* first_neighbour0: */ 7,
                   /* second_neighbour0: */ 8, /* first_neighbour1: */ 11,
                   /* second_neighbour1: */ 15);
      SetNeighbour(neighbour, /* index: */ 7, /* first_neighbour0: */ 1,
                   /* second_neighbour0: */ 4, /* first_neighbour1: */ 6,
                   /* second_neighbour1: */ 8);
      SetNeighbour(neighbour, /* index: */ 8, /* first_neighbour0: */ 6,
                   /* second_neighbour0: */ 7, /* first_neighbour1: */ 12,
                   /* second_neighbour1: */ 17);
      SetNeighbour(neighbour, /* index: */ 9, /* first_neighbour0: */ 10,
                   /* second_neighbour0: */ 11, /* first_neighbour1: */ 0,
                   /* second_neighbour1: */ 21);
      SetNeighbour(neighbour, /* index: */ 10, /* first_neighbour0: */ 9,
                   /* second_neighbour0: */ 11, /* first_neighbour1: */ 3,
                   /* second_neighbour1: */ 18);
      SetNeighbour(neighbour, /* index: */ 11, /* first_neighbour0: */ 9,
                   /* second_neighbour0: */ 10, /* first_neighbour1: */ 6,
                   /* second_neighbour1: */ 15);
      SetNeighbour(neighbour, /* index: */ 12, /* first_neighbour0: */ 13,
                   /* second_neighbour0: */ 14, /* first_neighbour1: */ 8,
                   /* second_neighbour1: */ 17);
      SetNeighbour(neighbour, /* index: */ 13, /* first_neighbour0: */ 12,
                   /* second_neighbour0: */ 14, /* first_neighbour1: */ 5,
                   /* second_neighbour1: */ 20);
      SetNeighbour(neighbour, /* index: */ 14, /* first_neighbour0: */ 12,
                   /* second_neighbour0: */ 13, /* first_neighbour1: */ 2,
                   /* second_neighbour1: */ 23);
      SetNeighbour(neighbour, /* index: */ 15, /* first_neighbour0: */ 6,
                   /* second_neighbour0: */ 11, /* first_neighbour1: */ 16,
                   /* second_neighbour1: */ 17);
      SetNeighbour(neighbour, /* index: */ 16, /* first_neighbour0: */ 15,
                   /* second_neighbour0: */ 17, /* first_neighbour1: */ 19,
                   /* second_neighbour1: */ 22);
      SetNeighbour(neighbour, /* index: */ 17, /* first_neighbour0: */ 15,
                   /* second_neighbour0: */ 16, /* first_neighbour1: */ 8,
                   /* second_neighbour1: */ 12);
      SetNeighbour(neighbour, /* index: */ 18, /* first_neighbour0: */ 3,
                   /* second_neighbour0: */ 10, /* first_neighbour1: */ 19,
                   /* second_neighbour1: */ 20);
      SetNeighbour(neighbour, /* index: */ 19, /* first_neighbour0: */ 18,
                   /* second_neighbour0: */ 20, /* first_neighbour1: */ 16,
                   /* second_neighbour1: */ 22);
      SetNeighbour(neighbour, /* index: */ 20, /* first_neighbour0: */ 5,
                   /* second_neighbour0: */ 13, /* first_neighbour1: */ 18,
                   /* second_neighbour1: */ 19);
      SetNeighbour(neighbour, /* index: */ 21, /* first_neighbour0: */ 0,
                   /* second_neighbour0: */ 9, /* first_neighbour1: */ 22,
                   /* second_neighbour1: */ 23);
      SetNeighbour(neighbour, /* index: */ 22, /* first_neighbour0: */ 16,
                   /* second_neighbour0: */ 19, /* first_neighbour1: */ 21,
                   /* second_neighbour1: */ 23);
      SetNeighbour(neighbour, /* index: */ 23, /* first_neighbour0: */ 2,
                   /* second_neighbour0: */ 14, /* first_neighbour1: */ 21,
                   /* second_neighbour1: */ 22);

      return neighbour;
    }();

} /* namespace muehle */
