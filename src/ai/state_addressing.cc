#include "muehle/ai/state_addressing.h"

#include <cassert>

#ifdef _MSC_VER
#include <limits>
#undef max
#endif /* _MSC_VER */

namespace muehle {

/* Initializes the state addressing. Thereby the precalculated variables are
 * loaded from the file pre_calculated_vars.dat. If the file does not exist, the
 * precalculated variables are calculated and saved into the file.
 * Directory - The directory where the pre_calculated_vars.dat file is stored */
StateAddressing::StateAddressing(std::wstring const& directory) {
  /* allocate memory */
  ResizeVector2D(amount_situations_c_d, GroupIndex{0},
                 NUM_STONES_PER_PLAYER + 1, NUM_STONES_PER_PLAYER + 1);
  ResizeVector2D(amount_situations_a_b, GroupIndex{0},
                 NUM_STONES_PER_PLAYER + 1, NUM_STONES_PER_PLAYER + 1);
  ResizeVector1D(group_index_a_b, GroupIndex{0},
                 MAX_NUM_SITUATIONS_A * MAX_NUM_SITUATIONS_B);
  ResizeVector1D(group_index_c_d, GroupIndex{0},
                 MAX_NUM_SITUATIONS_C * MAX_NUM_SITUATIONS_D);
  ResizeVector1D(symmetry_operation_c_d, SymOperationId{0},
                 MAX_NUM_SITUATIONS_C * MAX_NUM_SITUATIONS_D);
  ResizeVector2D(symmetry_transformation_table, 0u, NUM_SYM_OPERATIONS,
                 FieldStruct::size);
  ResizeVector3D(group_state_c_d, GroupStateNumber{0},
                 NUM_STONES_PER_PLAYER + 1, NUM_STONES_PER_PLAYER + 1, 1);
  ResizeVector3D(group_state_a_b, GroupStateNumber{0},
                 NUM_STONES_PER_PLAYER + 1, NUM_STONES_PER_PLAYER + 1, 1);
  ResizeVector1D(power_of_three, 0u, num_squares_group_c + num_squares_group_d);
  ResizeVector2D(m_over_n, 0u, FieldStruct::size + 1, FieldStruct::size + 1);
  ResizeVector1D(reverse_sym_operation, SymOperationId{0}, NUM_SYM_OPERATIONS);
  ResizeVector2D(conc_sym_operation, SymOperationId{0}, NUM_SYM_OPERATIONS,
                 NUM_SYM_OPERATIONS);
  ResizeVector3D(layer_index, LayerId{0}, 2, NUM_STONES_PER_PLAYER + 1,
                 NUM_STONES_PER_PLAYER + 1);
  ResizeVector1D(layer, LayerStruct{}, NUM_LAYERS);

  /* locals */
  CacheFile cf(directory, *this);

  /* vars already stored in file? */
  if (cf.ReadFromFile()) {
    return;
    /* calculate vars and save into file */
  } else {
    /* calc m_over_n */
    InitMOverN();

    /* power of three */
    InitPowerOfThree();

    /* int symmetry_operation_table, reverse_sym_operation */
    InitSymOperationMappings();

    /* init conc_sym_operation */
    InitConcSymOperation();

    /* init amount_situations_a_b, original_state_a_b, group_index_a_b */
    InitGroupAB();

    /* init amount_situations_c_d, original_state_c_d, symmetry_operation_c_d
     * and group_index_c_d */
    InitGroupCD();

    /* init layer_index and layer for moving phase */
    InitLayerRegardingMovingPhase();

    /* init layer_index and layer for setting phase */
    InitLayerRegardingSettingPhase();

    /* write cache to file */
    cf.WriteToFile();
  }
}

void StateAddressing::InitMOverN() {
  for (unsigned int m = 0; m <= FieldStruct::size; m++) {
    for (unsigned int n = 0; n <= FieldStruct::size; n++) {
      m_over_n[m][n] = (unsigned int)MOverNFunction(m, n);
    }
  }
}

void StateAddressing::InitPowerOfThree() {
  power_of_three[0] = 1;
  for (unsigned int i = 1; i < num_squares_group_c + num_squares_group_d; i++) {
    power_of_three[i] = 3 * power_of_three[i - 1];
  }
}

void StateAddressing::InitSymOperationMappings() {
  /* locals */
  FieldStruct::FieldPos i;

  /* symmetry operation table */
  for (i = 0; i < FieldStruct::size; i++) {
    symmetry_transformation_table[SO_TURN_LEFT][i] = so_table_turn_left[i];
    symmetry_transformation_table[SO_TURN_180][i] = so_table_turn_180[i];
    symmetry_transformation_table[SO_TURN_RIGHT][i] = so_table_turn_right[i];
    symmetry_transformation_table[SO_DO_NOTHING][i] = so_table_do_nothing[i];
    symmetry_transformation_table[SO_INVERT][i] = so_table_invert[i];
    symmetry_transformation_table[SO_MIRROR_VERT][i] = so_table_mirror_vert[i];
    symmetry_transformation_table[SO_MIRROR_HORI][i] = so_table_mirror_hori[i];
    symmetry_transformation_table[SO_MIRROR_DIAG_1][i] =
        so_table_mirror_diag_1[i];
    symmetry_transformation_table[SO_MIRROR_DIAG_2][i] =
        so_table_mirror_diag_2[i];
    symmetry_transformation_table[SO_INV_LEFT][i] = so_table_inv_left[i];
    symmetry_transformation_table[SO_INV_RIGHT][i] = so_table_inv_right[i];
    symmetry_transformation_table[SO_INV_180][i] = so_table_inv_180[i];
    symmetry_transformation_table[SO_INV_MIR_VERT][i] =
        so_table_inv_mir_hori[i];
    symmetry_transformation_table[SO_INV_MIR_HORI][i] =
        so_table_inv_mir_vert[i];
    symmetry_transformation_table[SO_INV_MIR_DIAG_1][i] =
        so_table_inv_mir_diag_1[i];
    symmetry_transformation_table[SO_INV_MIR_DIAG_2][i] =
        so_table_inv_mir_diag_2[i];
  }

  /* reverse symmetrie operation */
  reverse_sym_operation[SO_TURN_LEFT] = SO_TURN_RIGHT;
  reverse_sym_operation[SO_TURN_180] = SO_TURN_180;
  reverse_sym_operation[SO_TURN_RIGHT] = SO_TURN_LEFT;
  reverse_sym_operation[SO_DO_NOTHING] = SO_DO_NOTHING;
  reverse_sym_operation[SO_INVERT] = SO_INVERT;
  reverse_sym_operation[SO_MIRROR_VERT] = SO_MIRROR_VERT;
  reverse_sym_operation[SO_MIRROR_HORI] = SO_MIRROR_HORI;
  reverse_sym_operation[SO_MIRROR_DIAG_1] = SO_MIRROR_DIAG_1;
  reverse_sym_operation[SO_MIRROR_DIAG_2] = SO_MIRROR_DIAG_2;
  reverse_sym_operation[SO_INV_LEFT] = SO_INV_RIGHT;
  reverse_sym_operation[SO_INV_RIGHT] = SO_INV_LEFT;
  reverse_sym_operation[SO_INV_180] = SO_INV_180;
  reverse_sym_operation[SO_INV_MIR_VERT] = SO_INV_MIR_VERT;
  reverse_sym_operation[SO_INV_MIR_HORI] = SO_INV_MIR_HORI;
  reverse_sym_operation[SO_INV_MIR_DIAG_1] = SO_INV_MIR_DIAG_1;
  reverse_sym_operation[SO_INV_MIR_DIAG_2] = SO_INV_MIR_DIAG_2;
}

void StateAddressing::InitConcSymOperation() {
  /* locals*/
  FieldStruct::FieldPos i;
  SymOperationId sym_op_a, sym_op_b, sym_op_c;

  for (sym_op_a = 0; sym_op_a < NUM_SYM_OPERATIONS; sym_op_a++) {
    for (sym_op_b = 0; sym_op_b < NUM_SYM_OPERATIONS; sym_op_b++) {
      /* test each symmetry operation*/
      for (sym_op_c = 0; sym_op_c < NUM_SYM_OPERATIONS; sym_op_c++) {
        /* look if b(a(state)) == c(state)*/
        for (i = 0; i < FieldStruct::size; i++) {
          if (symmetry_transformation_table[sym_op_c][i] !=
              symmetry_transformation_table
                  [sym_op_a][symmetry_transformation_table[sym_op_b][i]]) {
            break;
          }
        }

        /* match found?*/
        if (i == FieldStruct::size) {
          conc_sym_operation[sym_op_a][sym_op_b] = sym_op_c;
          break;
        }
      }

      /* no match found*/
      assert((sym_op_c != NUM_SYM_OPERATIONS) &&
             "ERROR IN SYMMETRY-OPERATIONS");
    }
  }
}

void StateAddressing::ResizeGroupStateMappingArray(
    Vector3D<unsigned int>& group_state,
    const Vector2D<unsigned int>* p_amount_situations,
    unsigned int num_squares_in_group) const {
  /* locals*/
  NumWhiteStones nws;
  NumBlackStones nbs;
  GroupIndex amount_situations;

  for (nws = 0; nws <= NUM_STONES_PER_PLAYER; nws++) {
    for (nbs = 0; nbs <= NUM_STONES_PER_PLAYER; nbs++) {
      if (nws + nbs > num_squares_in_group) {
        continue;
      }
      amount_situations = (p_amount_situations != nullptr)
                              ? (*p_amount_situations)[nws][nbs]
                              : m_over_n[num_squares_in_group][nws] *
                                    m_over_n[num_squares_in_group - nws][nbs];
      group_state[nws][nbs].resize(amount_situations);
    }
  }
}

void StateAddressing::InitGroupAB() {
  /* locals*/
  unsigned int i;
  NumWhiteStones nws;
  NumBlackStones nbs;
  GroupStateNumber state_a_b;
  FieldStruct::FieldArray my_field;

  /* reserve memory*/
  ResizeGroupStateMappingArray(group_state_a_b, nullptr,
                               num_squares_group_a + num_squares_group_b);

  /* mark all group_index_c_d as not indexed*/
  group_index_a_b.assign(MAX_NUM_SITUATIONS_A * MAX_NUM_SITUATIONS_B,
                         NOT_INDEXED);

  /* iterate through each state within group A&B*/
  for (state_a_b = 0; state_a_b < MAX_NUM_SITUATIONS_A * MAX_NUM_SITUATIONS_B;
       state_a_b++) {
    /* new state ?*/
    if (group_index_a_b[state_a_b] != NOT_INDEXED) {
      continue;
    }

    /* zero field*/
    my_field.fill(PlayerId::square_is_free);

    /* make field*/
    CalcFieldBasedOnGroupAB(my_field, state_a_b);

    /* count black and white stones*/
    for (nws = 0, i = 0; i < FieldStruct::size; i++) {
      if (my_field[i] == FieldStruct::player_white) {
        nws++;
      }
    }
    for (nbs = 0, i = 0; i < FieldStruct::size; i++) {
      if (my_field[i] == FieldStruct::player_black) {
        nbs++;
      }
    }

    /* condition*/
    if (nws + nbs > num_squares_group_a + num_squares_group_b) {
      continue;
    }

    /* mark original state*/
    group_index_a_b[state_a_b] = amount_situations_a_b[nws][nbs];
    group_state_a_b[nws][nbs][group_index_a_b[state_a_b]] = state_a_b;

    /* state counter*/
    amount_situations_a_b[nws][nbs]++;
  }
}

void StateAddressing::InitGroupCD() {
  /* locals*/
  unsigned int i;
  NumWhiteStones nws;
  NumBlackStones nbs;
  GroupStateNumber state_c_d;
  GroupStateNumber sym_state_c_d;
  FieldStruct::FieldArray my_field;
  FieldStruct::FieldArray sym_field;
  Vector3D<GroupStateNumber> original_state_c_d_tmp;

  /* reserve memory*/
  ResizeVector3D(original_state_c_d_tmp, GroupStateNumber{0},
                 NUM_STONES_PER_PLAYER + 1, NUM_STONES_PER_PLAYER + 1, 1);
  ResizeGroupStateMappingArray(original_state_c_d_tmp, nullptr,
                               num_squares_group_c + num_squares_group_d);

  /* mark all group_index_c_d as not indexed*/
  group_index_c_d.assign(MAX_NUM_SITUATIONS_C * MAX_NUM_SITUATIONS_D,
                         NOT_INDEXED);

  /* iterate through each state within group C&D*/
  for (state_c_d = 0; state_c_d < MAX_NUM_SITUATIONS_C * MAX_NUM_SITUATIONS_D;
       state_c_d++) {
    /* new state ?*/
    if (group_index_c_d[state_c_d] != NOT_INDEXED) {
      continue;
    }

    /* zero field*/
    my_field.fill(PlayerId::square_is_free);

    /* make field */
    CalcFieldBasedOnGroupCD(my_field, state_c_d);

    /* count black and white stones */
    for (nws = 0, i = 0; i < FieldStruct::size; i++) {
      if (my_field[i] == FieldStruct::player_white) {
        nws++;
      }
    }
    for (nbs = 0, i = 0; i < FieldStruct::size; i++) {
      if (my_field[i] == FieldStruct::player_black) {
        nbs++;
      }
    }

    /* condition */
    if (nws + nbs > num_squares_group_c + num_squares_group_d) {
      continue;
    }
    if (nws > NUM_STONES_PER_PLAYER) {
      continue;
    }
    if (nbs > NUM_STONES_PER_PLAYER) {
      continue;
    }

    /* mark original state */
    group_index_c_d[state_c_d] = amount_situations_c_d[nws][nbs];
    symmetry_operation_c_d[state_c_d] = SO_DO_NOTHING;
    original_state_c_d_tmp[nws][nbs][group_index_c_d[state_c_d]] = state_c_d;

    /* mark all symmetric states */
    for (SymOperationId sym_op = 0; sym_op < NUM_SYM_OPERATIONS; sym_op++) {
      ApplySymmetryTransfToField(sym_op, false, my_field, sym_field);

      CalcGroupStateNumberCD(sym_field, sym_state_c_d);

      if (state_c_d != sym_state_c_d) {
        group_index_c_d[sym_state_c_d] = group_index_c_d[state_c_d];
        symmetry_operation_c_d[sym_state_c_d] = reverse_sym_operation[sym_op];
      }
    }

    /* state counter */
    amount_situations_c_d[nws][nbs]++;
  }

  /* copy from original_state_c_d_tmp to group_state_c_d */
  ResizeGroupStateMappingArray(group_state_c_d, &amount_situations_c_d,
                               num_squares_group_c + num_squares_group_d);
  for (nws = 0; nws <= NUM_STONES_PER_PLAYER; nws++) {
    for (nbs = 0; nbs <= NUM_STONES_PER_PLAYER; nbs++) {
      if (nws + nbs > num_squares_group_c + num_squares_group_d) {
        continue;
      }
      for (i = 0; i < amount_situations_c_d[nws][nbs]; i++) {
        group_state_c_d[nws][nbs][i] = original_state_c_d_tmp[nws][nbs][i];
      }
    }
  }
}

void StateAddressing::InitLayerRegardingMovingPhase() {
  /* locals */
  NumWhiteStones nws;
  NumBlackStones nbs;
  unsigned int total_num_stones;
  LayerId layer_num;
  NumWhiteStones w_c_d,
      w_a_b; /* number of white and black stones for group C&D and A&B */
  NumBlackStones b_c_d, b_a_b;

  /* iterate through each layer */
  for (total_num_stones = 0, layer_num = 0;
       total_num_stones <= 2 * NUM_STONES_PER_PLAYER; total_num_stones++) {
    /* iterate through each number of white and black stones */
    for (nws = 0; nws <= total_num_stones; nws++) {
      for (nbs = 0; nbs <= total_num_stones - nws; nbs++) {
        /* conditions */
        if (nws > NUM_STONES_PER_PLAYER) {
          continue;
        }
        if (nbs > NUM_STONES_PER_PLAYER) {
          continue;
        }
        if (nws + nbs != total_num_stones) {
          continue;
        }

        /* set layer properties */
        layer_index[LAYER_INDEX_MOVING_PHASE][nws][nbs] = layer_num;
        layer[layer_num].amount_white_stones = nws;
        layer[layer_num].amount_black_stones = nbs;
        layer[layer_num].num_sub_layers = 0;

        /* iterate through each number of white and black stones for group C&D
         */
        SubLayerId cur_sub_layer_id = 0;
        GroupIndex cur_group_index_offset = 0;
        for (w_c_d = 0; w_c_d <= layer[layer_num].amount_white_stones;
             w_c_d++) {
          for (b_c_d = 0; b_c_d <= layer[layer_num].amount_black_stones;
               b_c_d++) {
            /* calc number of white and black stones for group A&B */
            w_a_b = layer[layer_num].amount_white_stones - w_c_d;
            b_a_b = layer[layer_num].amount_black_stones - b_c_d;

            /* conditions */
            if (w_c_d + w_a_b != layer[layer_num].amount_white_stones) {
              continue;
            }
            if (b_c_d + b_a_b != layer[layer_num].amount_black_stones) {
              continue;
            }
            if (w_a_b + b_a_b > num_squares_group_a + num_squares_group_b) {
              continue;
            }
            if (w_c_d + b_c_d > num_squares_group_c + num_squares_group_d) {
              continue;
            }

            layer[layer_num].sub_layer[cur_sub_layer_id].min_ndex =
                cur_group_index_offset;
            layer[layer_num].sub_layer[cur_sub_layer_id].max_index =
                cur_group_index_offset +
                amount_situations_a_b[w_a_b][b_a_b] *
                    amount_situations_c_d[w_c_d][b_c_d] -
                1;
            layer[layer_num]
                .sub_layer[cur_sub_layer_id]
                .num_black_stones_group_a_b = b_a_b;
            layer[layer_num]
                .sub_layer[cur_sub_layer_id]
                .num_black_stones_group_c_d = b_c_d;
            layer[layer_num]
                .sub_layer[cur_sub_layer_id]
                .num_white_stones_group_a_b = w_a_b;
            layer[layer_num]
                .sub_layer[cur_sub_layer_id]
                .num_white_stones_group_c_d = w_c_d;
            layer[layer_num].sub_layer_index_a_b[w_a_b][b_a_b] = NOT_INDEXED;
            layer[layer_num].sub_layer_index_c_d[w_c_d][b_c_d] =
                cur_sub_layer_id;
            layer[layer_num].num_sub_layers++;
            cur_group_index_offset += amount_situations_a_b[w_a_b][b_a_b] *
                                      amount_situations_c_d[w_c_d][b_c_d];
            cur_sub_layer_id++;
          }
        }

        /* next layer */
        layer_num++;
      }
    }
  }
}

void StateAddressing::InitLayerRegardingSettingPhase() {
  /* locals */
  NumWhiteStones nws;            /* number of white stones */
  NumBlackStones nbs;            /* number of black stones */
  unsigned int total_num_stones; /* nws + nbs */
  LayerId layer_num;             /* layer number */
  NumWhiteStones w_c_d,
      w_a_b; /* number of white stones for group C&D and A&B */
  NumBlackStones b_c_d,
      b_a_b; /* number of black stones for group C&D and A&B */

  /* iterate through each layer */
  for (total_num_stones = 0, layer_num = NUM_LAYERS - 1;
       total_num_stones <= 2 * NUM_STONES_PER_PLAYER; total_num_stones++) {
    /* iterate through each number of white and black stones */
    for (nws = 0; nws <= total_num_stones; nws++) {
      for (nbs = 0; nbs <= total_num_stones - nws; nbs++) {
        /* conditions */
        if (nws > NUM_STONES_PER_PLAYER) {
          continue;
        }
        if (nbs > NUM_STONES_PER_PLAYER) {
          continue;
        }
        if (nws + nbs != total_num_stones) {
          continue;
        }

        /* set layer properties */
        layer[layer_num].amount_white_stones = nws;
        layer[layer_num].amount_black_stones = nbs;
        layer_index[LAYER_INDEX_SETTING_PHASE][nws][nbs] = layer_num;
        layer[layer_num].num_sub_layers = 0;

        /* iterate through each number of white and black stones for group C&D
         */
        SubLayerId cur_sub_layer_id = 0;
        GroupIndex cur_group_index_offset = 0;
        for (w_c_d = 0; w_c_d <= layer[layer_num].amount_white_stones;
             w_c_d++) {
          for (b_c_d = 0; b_c_d <= layer[layer_num].amount_black_stones;
               b_c_d++) {
            /* calc number of white and black stones for group A&B */
            w_a_b = layer[layer_num].amount_white_stones - w_c_d;
            b_a_b = layer[layer_num].amount_black_stones - b_c_d;

            /* conditions */
            if (w_c_d + w_a_b != layer[layer_num].amount_white_stones) {
              continue;
            }
            if (b_c_d + b_a_b != layer[layer_num].amount_black_stones) {
              continue;
            }
            if (w_a_b + b_a_b > num_squares_group_a + num_squares_group_b) {
              continue;
            }
            if (w_c_d + b_c_d > num_squares_group_c + num_squares_group_d) {
              continue;
            }

            layer[layer_num].sub_layer[cur_sub_layer_id].min_ndex =
                cur_group_index_offset;
            layer[layer_num].sub_layer[cur_sub_layer_id].max_index =
                cur_group_index_offset +
                amount_situations_a_b[w_a_b][b_a_b] *
                    amount_situations_c_d[w_c_d][b_c_d] -
                1;
            layer[layer_num]
                .sub_layer[cur_sub_layer_id]
                .num_black_stones_group_a_b = b_a_b;
            layer[layer_num]
                .sub_layer[cur_sub_layer_id]
                .num_black_stones_group_c_d = b_c_d;
            layer[layer_num]
                .sub_layer[cur_sub_layer_id]
                .num_white_stones_group_a_b = w_a_b;
            layer[layer_num]
                .sub_layer[cur_sub_layer_id]
                .num_white_stones_group_c_d = w_c_d;
            layer[layer_num].sub_layer_index_a_b[w_a_b][b_a_b] = NOT_INDEXED;
            layer[layer_num].sub_layer_index_c_d[w_c_d][b_c_d] =
                cur_sub_layer_id;
            layer[layer_num].num_sub_layers++;
            cur_group_index_offset += amount_situations_a_b[w_a_b][b_a_b] *
                                      amount_situations_c_d[w_c_d][b_c_d];
            cur_sub_layer_id++;
          }
        }

        /* next layer */
        layer_num--;
      }
    }
  }
}

/* Returns the number of possibilities to put n different stones in m holes */
long long StateAddressing::MOverNFunction(unsigned int m, unsigned int n) {
  /* locals */
  long long result = 1;
  long long fak_n = 1;
  unsigned int i;

  /* invalid parameters ? */
  if (n > m) {
    return 0;
  }

  /* flip, since then the result value won't get so high */
  if (n > m / 2) {
    n = m - n;
  }

  /* calc number of possibilities one can put n different stones in m holes */
  for (i = m - n + 1; i <= m; i++) {
    result *= i;
  }

  /* calc number of possibilities one can sort n different stones */
  for (i = 1; i <= n; i++) {
    fak_n *= i;
  }

  /* divide */
  result /= fak_n;

  return result;
}

/* Updates the corresponding field array based on the given state number within
 * a group */
inline void StateAddressing::CalcFieldBasedOnGroup(
    FieldStruct::FieldArray& field, unsigned int num_squares_in_group,
    GroupStateNumber state_number, const unsigned int* square_index_group,
    unsigned int group_order, const Vector1D<unsigned int>& power_of_three) {
  for (unsigned int j = 0; j < num_squares_in_group; ++j) {
    field[square_index_group[j]] = static_cast<PlayerId>(
        (state_number / power_of_three[group_order - j]) % 3);
  }
}

void StateAddressing::CalcFieldBasedOnGroupAB(
    FieldStruct::FieldArray& field, GroupStateNumber state_a_b) const {
  CalcFieldBasedOnGroup(field, num_squares_group_a, state_a_b,
                        square_index_group_a, group_order_a, power_of_three);
  CalcFieldBasedOnGroup(field, num_squares_group_b, state_a_b,
                        square_index_group_b, group_order_b, power_of_three);
}

void StateAddressing::CalcFieldBasedOnGroupCD(
    FieldStruct::FieldArray& field, GroupStateNumber state_c_d) const {
  CalcFieldBasedOnGroup(field, num_squares_group_c, state_c_d,
                        square_index_group_c, group_order_c, power_of_three);
  CalcFieldBasedOnGroup(field, num_squares_group_d, state_c_d,
                        square_index_group_d, group_order_d, power_of_three);
}

/* Calculates the state number based on the field array within a group */
inline void StateAddressing::CalcGroupStateNumberBasedOnField(
    const FieldStruct::FieldArray& field, unsigned int num_squares_in_group,
    GroupStateNumber& state_number, const unsigned int* square_index_group,
    unsigned int group_order, const Vector1D<unsigned int>& power_of_three) {
  for (int j = 0; j < num_squares_in_group; ++j) {
    state_number += static_cast<StateId>(field[square_index_group[j]]) *
                    power_of_three[group_order - j];
  }
}

void StateAddressing::CalcGroupStateNumberAB(
    const FieldStruct::FieldArray& field,
    GroupStateNumber& state_number_a_b) const {
  state_number_a_b = 0;
  CalcGroupStateNumberBasedOnField(field, num_squares_group_a, state_number_a_b,
                                   square_index_group_a, group_order_a,
                                   power_of_three);
  CalcGroupStateNumberBasedOnField(field, num_squares_group_b, state_number_a_b,
                                   square_index_group_b, group_order_b,
                                   power_of_three);
}

void StateAddressing::CalcGroupStateNumberCD(
    const FieldStruct::FieldArray& field,
    GroupStateNumber& state_number_c_d) const {
  state_number_c_d = 0;
  CalcGroupStateNumberBasedOnField(field, num_squares_group_c, state_number_c_d,
                                   square_index_group_c, group_order_c,
                                   power_of_three);
  CalcGroupStateNumberBasedOnField(field, num_squares_group_d, state_number_c_d,
                                   square_index_group_d, group_order_d,
                                   power_of_three);
}

/* Applies a symmetrie operation on a source_field returning dest_field */
void StateAddressing::ApplySymmetryTransfToField(
    SymOperationId symmetry_operation_number, bool do_inverse_operation,
    const FieldStruct::FieldArray& source_field,
    FieldStruct::FieldArray& dest_field) const {
  symmetry_operation_number =
      do_inverse_operation ? reverse_sym_operation[symmetry_operation_number]
                           : symmetry_operation_number;
  auto& sym_map = symmetry_transformation_table[symmetry_operation_number];
  for (FieldStruct::FieldPos i = 0; i < FieldStruct::size; i++) {
    dest_field[i] = source_field[sym_map[i]];
  }
}

/* Applies a symmetrie operation on a source_field returning dest_field */
void StateAddressing::ApplySymmetryTransfToField(
    SymOperationId symmetry_operation_number, bool do_inverse_operation,
    const FieldStruct::MillArray& source_field,
    FieldStruct::MillArray& dest_field) const {
  symmetry_operation_number =
      do_inverse_operation ? reverse_sym_operation[symmetry_operation_number]
                           : symmetry_operation_number;
  auto& sym_map = symmetry_transformation_table[symmetry_operation_number];
  for (FieldStruct::FieldPos i = 0; i < FieldStruct::size; i++) {
    dest_field[i] = source_field[sym_map[i]];
  }
}

/* Applies a symmetrie operation on a field */
bool StateAddressing::ApplySymmetryTransfToField(
    SymOperationId symmetry_operation_number, bool do_inverse_operation,
    FieldStruct& field) const {
  /* checks */
  if (symmetry_operation_number >= NUM_SYM_OPERATIONS) {
    return false;
  }

  /* apply symmetrie operation on field */
  FieldStruct::FieldArray tmp_field = field.field;
  ApplySymmetryTransfToField(symmetry_operation_number, do_inverse_operation,
                             tmp_field, field.field);

  /* ... and mill counter if necessary */
  FieldStruct::MillArray tmp_stone_part_of_mill = field.stone_part_of_mill;
  ApplySymmetryTransfToField(symmetry_operation_number, do_inverse_operation,
                             tmp_stone_part_of_mill, field.stone_part_of_mill);

  return true;
}

/* Applies a symmetrie operation on a field */
bool StateAddressing::ApplySymmetryTransfToField(
    SymOperationId symmetry_operation_number, bool do_inverse_operation,
    FieldStruct::Core& field) const {
  /* checks */
  if (symmetry_operation_number >= NUM_SYM_OPERATIONS) {
    return false;
  }

  /* apply symmetrie operation on field */
  FieldStruct::FieldArray tmp_field = field.field;
  ApplySymmetryTransfToField(symmetry_operation_number, do_inverse_operation,
                             tmp_field, field.field);

  return true;
}

/* Returns the layer number for a given number of white and black stones */
StateAddressing::LayerId StateAddressing::GetLayerNumber(
    const FieldStruct::Core& field) const {
  return GetLayerNumber(field.cur_player.num_stones,
                        field.opp_player.num_stones, field.setting_phase);
}

/* Returns the layer number for a given number of white and black stones
 * It is assumed that the current player is always player white (2). */
StateAddressing::LayerId StateAddressing::GetLayerNumber(
    unsigned int num_stones_of_cur_player,
    unsigned int num_stones_of_opp_player, bool is_setting_phase) const {
  const unsigned int phase_index =
      is_setting_phase ? LAYER_INDEX_SETTING_PHASE : LAYER_INDEX_MOVING_PHASE;
  const NumWhiteStones nws = num_stones_of_cur_player;
  const NumBlackStones nbs = num_stones_of_opp_player;
  return layer_index[phase_index][nws][nbs];
}

/* Adapts the field array to the current player, so that current player is
 * always player white (2)
 * This is necessary for the state addressing, since the
 * state addressing assumes that the current player is always player white (2)
 */
void StateAddressing::AdaptFieldArrayToCurPlayer(
    const FieldStruct::FieldArray& src_field,
    FieldStruct::FieldArray& dst_field, PlayerId cur_player) const {
  for (FieldStruct::FieldPos i = 0; i < FieldStruct::size; i++) {
    if (src_field[i] == PlayerId::square_is_free) {
      dst_field[i] = PlayerId::square_is_free;
    } else if (src_field[i] == cur_player) {
      dst_field[i] = FieldStruct::player_white;
    } else {
      dst_field[i] = FieldStruct::player_black;
    }
  }
}

/* Counts the number of white and black stones in a group */
void StateAddressing::CountStonesInGroup(
    const FieldStruct::Core& field, NumWhiteStones& num_white_stones_group_a_b,
    NumBlackStones& num_black_stones_group_a_b,
    NumWhiteStones& num_white_stones_group_c_d,
    NumBlackStones& num_black_stones_group_c_d) const {
  num_white_stones_group_a_b = 0;
  num_black_stones_group_a_b = 0;
  num_white_stones_group_c_d = 0;
  num_black_stones_group_c_d = 0;

  for (FieldStruct::FieldPos i = 0; i < FieldStruct::size; i++) {
    if (field.GetStone(i) == field.GetCurPlayer().id) {
      if (field_pos_is_of_group[i] == GROUP_A) {
        num_white_stones_group_a_b++;
      }
      if (field_pos_is_of_group[i] == GROUP_B) {
        num_white_stones_group_a_b++;
      }
      if (field_pos_is_of_group[i] == GROUP_C) {
        num_white_stones_group_c_d++;
      }
      if (field_pos_is_of_group[i] == GROUP_D) {
        num_white_stones_group_c_d++;
      }
    } else if (field.GetStone(i) == field.GetOppPlayer().id) {
      if (field_pos_is_of_group[i] == GROUP_A) {
        num_black_stones_group_a_b++;
      }
      if (field_pos_is_of_group[i] == GROUP_B) {
        num_black_stones_group_a_b++;
      }
      if (field_pos_is_of_group[i] == GROUP_C) {
        num_black_stones_group_c_d++;
      }
      if (field_pos_is_of_group[i] == GROUP_D) {
        num_black_stones_group_c_d++;
      }
    }
  }
}

/* Returns the state number for a given field */
bool StateAddressing::GetStateNumber(LayerId layer_num, StateId& state_number,
                                     SymOperationId& sym_op,
                                     const FieldStruct::Core& field) const {
  /* locals */
  FieldStruct::FieldArray my_field;
  FieldStruct::FieldArray sym_field;
  NumWhiteStones w_c_d;
  NumBlackStones b_c_d;
  GroupStateNumber state_a_b;
  GroupStateNumber state_c_d;

  /* the state numbers assumes that the current player is always player white */
  /* (2) thus we have to convert the field to this assumption */
  AdaptFieldArrayToCurPlayer(field.field, my_field, field.GetCurPlayer().id);

  /* count stones in each group */
  CountStonesInGroupCD(field, w_c_d, b_c_d);

  /* calc state_c_d */
  CalcGroupStateNumberCD(my_field, state_c_d);

  /* apply symmetry operation on group A&B */
  ApplySymmetryTransfToField(symmetry_operation_c_d[state_c_d], false, my_field,
                             sym_field);

  /* calc state_a_b
   * Optimized: unroll loop and use pointer arithmetic for better cache locality
   */
  state_a_b = 0;
  const auto* sq_a = square_index_group_a;
  const auto* sq_b = square_index_group_b;
  const auto* p3 = power_of_three.data();
  for (unsigned int i = 0; i < num_squares_group_a; ++i) {
    state_a_b += static_cast<StateId>(sym_field[square_index_group_a[i]]) *
                 p3[group_order_a - i];
  }
  for (unsigned int i = 0; i < num_squares_group_b; ++i) {
    state_a_b += static_cast<StateId>(sym_field[square_index_group_b[i]]) *
                 p3[group_order_b - i];
  }

  /* calc index */
  const unsigned int state_number_with_in_sub_layer =
      group_index_a_b[state_a_b] * amount_situations_c_d[w_c_d][b_c_d] +
      group_index_c_d[state_c_d];
  const SubLayerId sub_layer_index_c_d =
      layer[layer_num].sub_layer_index_c_d[w_c_d][b_c_d];
  state_number = (layer[layer_num].sub_layer[sub_layer_index_c_d].min_ndex +
                  state_number_with_in_sub_layer);
  sym_op = symmetry_operation_c_d[state_c_d];

  /* consider offset based on total_num_missing_stones */
  if (IsSettingPhase(layer_num)) {
    if (!AddTotalNumMissingStonesOffset(state_number, field)) {
      return false;
    }
  }

  return true;
}

/* Counts the number of stones in group C and D */
void StateAddressing::CountStonesInGroupCD(const FieldStruct::Core& field,
                                           NumWhiteStones& w_c_d,
                                           NumBlackStones& b_c_d) const {
  /* locals */
  const PlayerId* field_ptr = field.field.data();
  PlayerId cur_player_id = field.GetCurPlayer().id;
  PlayerId opp_player_id = field.GetOppPlayer().id;

  /* reset */
  w_c_d = 0;
  b_c_d = 0;

  /* count stones in each group Use pointers and avoid repeated lookups for
   * better cache locality and speed */
  const auto* group_ptr = field_pos_is_of_group;
  for (FieldStruct::FieldPos i = 0; i < FieldStruct::size; ++i) {
    const auto g = group_ptr[i];
    if (g == GROUP_C || g == GROUP_D) {
      const auto f = field_ptr[i];
      if (f == cur_player_id) {
        ++w_c_d;
      } else if (f == opp_player_id) {
        ++b_c_d;
      }
    }
  }
}

/* Adds the statenumber offset based on the total number of missing stones */
bool StateAddressing::AddTotalNumMissingStonesOffset(
    StateId& state_number, const FieldStruct::Core& field) const {
  /* locals */
  unsigned int nsm_cur_player = field.GetCurPlayer().num_stones_missing;
  unsigned int nsm_opp_player = field.GetOppPlayer().num_stones_missing;
  unsigned int total_num_missing_stones = nsm_cur_player + nsm_opp_player;

  /* at maximum 2 stones can be removed from closed mills in total */
  if (nsm_cur_player > FieldStruct::num_stones_per_player ||
      nsm_opp_player > FieldStruct::num_stones_per_player) {
    return false;
  }

  /* add offset */
  state_number = state_number * GetMaxTotalNumMissingStones(
                                    field.GetCurPlayer().num_stones,
                                    field.GetOppPlayer().num_stones) +
                 total_num_missing_stones;

  return true;
}

/* Returns the maximum total number of missing stones for a given state number
 * and layer. */
unsigned int StateAddressing::GetMaxTotalNumMissingStones(
    NumWhiteStones amount_white_stones, NumBlackStones amount_black_stones) {
  if (amount_white_stones > FieldStruct::num_stones_per_player) {
    return 0;
  }
  if (amount_black_stones > FieldStruct::num_stones_per_player) {
    return 0;
  }
  return 2 * FieldStruct::num_stones_per_player - amount_white_stones -
         amount_black_stones;
}

/* Checks if the given layer is in the setting phase */
bool StateAddressing::IsSettingPhase(LayerId layer_num) const {
  return layer_num >= NUM_LAYERS / 2;
}

/* Returns the field for a given state number and layer. Thereby the current
 * player can be chosen. */
bool StateAddressing::GetFieldByStateNumber(LayerId layer_num,
                                            StateId state_number,
                                            FieldStruct& field,
                                            PlayerId cur_player) const {
  /* locals */
  const bool setting_phase = IsSettingPhase(layer_num);
  const LayerStruct& cur_layer = layer[layer_num];
  const unsigned int total_num_missing_stones = GetTotalNumMissingStones(
      state_number, setting_phase, cur_layer.amount_white_stones,
      cur_layer.amount_black_stones);
  unsigned int state_number_with_in_sub_layer;
  GroupIndex index_with_in_group_a_b;
  GroupIndex index_with_in_group_c_d;
  GroupStateNumber state_a_b, state_c_d;
  SubLayerId sub_layer_index_c_d;
  NumWhiteStones w_a_b, w_c_d;
  NumBlackStones b_a_b, b_c_d;
  FieldStruct::FieldArray my_field;
  FieldStruct::FieldArray sym_field;

  /* get w_c_d, b_c_d, w_a_b, b_a_b */
  cur_layer.GetNumGroupStonesByStateNumber(state_number, setting_phase, w_a_b,
                                           b_a_b, w_c_d, b_c_d);

  /* get index within groups */
  sub_layer_index_c_d = cur_layer.sub_layer_index_c_d[w_c_d][b_c_d];
  state_number_with_in_sub_layer =
      cur_layer.GetStateNumberWithInSubLayer(state_number, setting_phase) -
      cur_layer.sub_layer[sub_layer_index_c_d].min_ndex;
  index_with_in_group_a_b =
      state_number_with_in_sub_layer / amount_situations_c_d[w_c_d][b_c_d];
  index_with_in_group_c_d =
      state_number_with_in_sub_layer % amount_situations_c_d[w_c_d][b_c_d];

  /* get state within groups */
  state_c_d = group_state_c_d[w_c_d][b_c_d][index_with_in_group_c_d];
  state_a_b = group_state_a_b[w_a_b][b_a_b][index_with_in_group_a_b];

  /* set my_field from state_a_b */
  CalcFieldBasedOnGroupAB(my_field, state_a_b);

  /* apply symmetry operation on group A&B */
  ApplySymmetryTransfToField(symmetry_operation_c_d[state_c_d], true, my_field,
                             sym_field);

  /* set my_field from state_c_d */
  CalcFieldBasedOnGroupCD(sym_field, state_c_d);

  /* the state numbers assumes that the current player is always player white
   * thus we have to convert this assumption to the requested player */
  AdaptFieldArrayToCurPlayer(sym_field, my_field, cur_player);

  /* set field */
  field.Reset(cur_player);
  return field.SetSituation(my_field, setting_phase, total_num_missing_stones);
}

unsigned int StateAddressing::LayerStruct::GetStateNumberWithInSubLayer(
    StateId state_number, bool setting_phase) const {
  return setting_phase
             ? state_number / StateAddressing::GetMaxTotalNumMissingStones(
                                  amount_white_stones, amount_black_stones)
             : state_number;
}

/* Returns the total number of missing stones for a given state */
unsigned int StateAddressing::GetTotalNumMissingStones(
    StateId state_number, bool setting_phase,
    NumWhiteStones amount_white_stones,
    NumBlackStones amount_black_stones) const {
  return setting_phase
             ? state_number % GetMaxTotalNumMissingStones(amount_white_stones,
                                                          amount_black_stones)
             : 0;
}

void StateAddressing::LayerStruct::GetNumGroupStonesByStateNumber(
    StateId state_number, bool setting_phase,
    NumWhiteStones& num_white_stones_group_a_b,
    NumBlackStones& num_black_stones_group_a_b,
    NumWhiteStones& num_white_stones_group_c_d,
    NumBlackStones& num_black_stones_group_c_d) const {
  unsigned int state_number_with_in_sub_layer =
      GetStateNumberWithInSubLayer(state_number, setting_phase);
  for (SubLayerId sub_layer_index_c_d = 0;
       sub_layer_index_c_d <= num_sub_layers; sub_layer_index_c_d++) {
    if (sub_layer[sub_layer_index_c_d].min_ndex <=
            state_number_with_in_sub_layer &&
        sub_layer[sub_layer_index_c_d].max_index >=
            state_number_with_in_sub_layer) {
      num_white_stones_group_c_d =
          sub_layer[sub_layer_index_c_d].num_white_stones_group_c_d;
      num_black_stones_group_c_d =
          sub_layer[sub_layer_index_c_d].num_black_stones_group_c_d;
      num_white_stones_group_a_b =
          sub_layer[sub_layer_index_c_d].num_white_stones_group_a_b;
      num_black_stones_group_a_b =
          sub_layer[sub_layer_index_c_d].num_black_stones_group_a_b;
      break;
    }
  }
}

/* Returns true if a given symmetry operation would not change the field */
bool StateAddressing::IsSymOperationInvariant(
    SymOperationId symmetry_operation, const FieldStruct::Core& field) const {
  if (symmetry_operation >= NUM_SYM_OPERATIONS) {
    return false;
  }
  if (symmetry_operation == StateAddressing::SO_DO_NOTHING) {
    return true;
  }
  const auto* the_field = field.field.data();
  const auto* sym_trans_table =
      symmetry_transformation_table[symmetry_operation].data();
  /* Unroll loop for better speed, avoid repeated lookups */
  for (unsigned int i = 0; i < 8; ++i) {
    const auto c = square_index_group_c[i];
    const auto d = square_index_group_d[i];
    if (the_field[c] != the_field[sym_trans_table[c]]) {
      return false;
    }
    if (the_field[d] != the_field[sym_trans_table[d]]) {
      return false;
    }
  }
  return true;
}

/* Returns the layer containing further information for a given number */
const StateAddressing::LayerStruct& StateAddressing::GetLayer(
    LayerId layer_num) const {
  return layer[layer_num];
}

/* Returns the number of knots in a given layer */
unsigned int StateAddressing::GetNumberOfKnotsInLayer(LayerId layer_num) const {
  /* checks */
  if (layer_num >= StateAddressing::NUM_LAYERS) {
    return 0;
  }

  /* locals */
  unsigned int number_of_knots =
      layer[layer_num]
          .sub_layer[layer[layer_num].num_sub_layers - 1]
          .max_index +
      1;

  /* during setting phase removal of stones from closed mills lead to different
   * states, which must be distuinguished */
  if (IsSettingPhase(layer_num)) {
    /* since the white player always starts, there cannot be 9 white stones on
     * the board, during setting phase */
    if (layer[layer_num].amount_white_stones >=
        FieldStruct::num_stones_per_player) {
      return 0;
    }

    /* consider offset based on total_num_missing_stones use uint64_t to avoid
     * overflow */
    uint64_t knots64 =
        static_cast<uint64_t>(number_of_knots) *
        GetMaxTotalNumMissingStones(layer[layer_num].amount_white_stones,
                                    layer[layer_num].amount_black_stones);
    if (knots64 > std::numeric_limits<unsigned int>::max()) {
      std::cout << "Error: Number of knots " << knots64
                << " exceeds unsigned int range for layer " << layer_num
                << std::endl;
      assert(false && "Number of knots exceeds unsigned int range");
    }
    number_of_knots = static_cast<unsigned int>(knots64);
  }

  /* during moving phase, we have to check if the layer is reachable return zero
   * if layer is not reachable */
  if (((layer[layer_num].amount_black_stones < 2 ||
        layer[layer_num].amount_white_stones < 2) &&
       layer_num < 100) ||
      (layer[layer_num].amount_black_stones == 2 &&
       layer[layer_num].amount_white_stones == 2 && layer_num < 100) ||
      (layer_num == 100)) {
    return 0;
  }

  /* another way */
  return number_of_knots;
}

/* Returns the state numbers of all symmetric states for a given field */
bool StateAddressing::GetStateNumbersOfSymmetricStates(
    const FieldStruct::Core& field,
    std::array<StateId, NUM_SYM_OPERATIONS>& state_numbers) const {
  /* save current field */
  SymOperationId symmetry_operation;
  SymOperationId sym_op_applied;
  FieldStruct::Core sym_field;
  LayerId layer_number = GetLayerNumber(field);
  StateId state_number;

  /* get state number of current field */
  if (!GetStateNumber(layer_number, state_number, symmetry_operation, field)) {
    return false;
  }
  sym_field = field;

  /* add all symmetric states */
  for (symmetry_operation = 0;
       symmetry_operation < StateAddressing::NUM_SYM_OPERATIONS;
       symmetry_operation++) {
    /* set state number to the state without any sym operation */
    state_numbers[symmetry_operation] = state_number;

    /* TODO: check if this is really correct. Shouldn't it be rather: if
     * (symmetry_operation != SO_DO_NOTHING &&
     * IsSymOperationInvariant(symmetry_operation, field)) { continue; } only
     * add if sym operation is invariant */
    if (!IsSymOperationInvariant(symmetry_operation, field)) {
      continue;
    }

    /* appy symmetry operation */
    ApplySymmetryTransfToField(symmetry_operation, false, field.field,
                               sym_field.field);

    /* store state number */
    if (!GetStateNumber(layer_number, state_numbers[symmetry_operation],
                        sym_op_applied, sym_field)) {
      return false;
    }
  }
  return true;
}

/* Constructor */
StateAddressing::CacheFile::CacheFile(std::wstring const& directory,
                                      StateAddressing& sa)
    : sa(sa) {
  /* locals */
  std::filesystem::path base(directory);
  if (!base.empty()) {
    std::filesystem::create_directories(base);
  }
  file_path = (base / L"pre_calculated_vars.dat").wstring();
  file.open(std::filesystem::path(file_path),
            std::ios::in | std::ios::out | std::ios::binary);
  if (!file.is_open()) {
    file.clear();
    file.open(std::filesystem::path(file_path),
              std::ios::out | std::ios::binary);
    file.close();
    file.open(std::filesystem::path(file_path),
              std::ios::in | std::ios::out | std::ios::binary);
  }
}

StateAddressing::CacheFile::~CacheFile() {
  if (file.is_open()) {
    file.close();
  }
}

/* Reads the precalculated variables from the file pre_calculated_vars.dat */
bool StateAddressing::CacheFile::ReadFromFile() {
  /* locals */
  if (!file.is_open()) {
    return false;
  }
  file.clear();
  file.seekg(0);
  file.read(reinterpret_cast<char*>(&header), sizeof(header));

  /* check if file is valid */
  if (header.size_in_bytes != sizeof(header)) {
    return false;
  }

  if (!ReadVector(file, sa.layer) || !ReadVector(file, sa.layer_index) ||
      !ReadVector(file, sa.amount_situations_a_b) ||
      !ReadVector(file, sa.amount_situations_c_d) ||
      !ReadVector(file, sa.group_index_a_b) ||
      !ReadVector(file, sa.group_index_c_d) ||
      !ReadVector(file, sa.symmetry_operation_c_d) ||
      !ReadVector(file, sa.power_of_three) ||
      !ReadVector(file, sa.symmetry_transformation_table) ||
      !ReadVector(file, sa.reverse_sym_operation) ||
      !ReadVector(file, sa.conc_sym_operation) ||
      !ReadVector(file, sa.m_over_n)) {
    return false;
  }
  sa.ResizeGroupStateMappingArray(sa.group_state_a_b, nullptr,
                                  num_squares_group_a + num_squares_group_b);
  sa.ResizeGroupStateMappingArray(sa.group_state_c_d, &sa.amount_situations_c_d,
                                  num_squares_group_c + num_squares_group_d);
  return ReadVector(file, sa.group_state_a_b) &&
         ReadVector(file, sa.group_state_c_d);
}

/* Writes the precalculated variables to the file pre_calculated_vars.dat */
bool StateAddressing::CacheFile::WriteToFile() {
  /* locals */
  if (!file.is_open()) {
    return false;
  }
  file.clear();
  file.seekp(0);

  /* write vars into file */
  header.size_in_bytes = sizeof(header);

  file.write(reinterpret_cast<const char*>(&header), header.size_in_bytes);
  return WriteVector(file, sa.layer) && WriteVector(file, sa.layer_index) &&
         WriteVector(file, sa.amount_situations_a_b) &&
         WriteVector(file, sa.amount_situations_c_d) &&
         WriteVector(file, sa.group_index_a_b) &&
         WriteVector(file, sa.group_index_c_d) &&
         WriteVector(file, sa.symmetry_operation_c_d) &&
         WriteVector(file, sa.power_of_three) &&
         WriteVector(file, sa.symmetry_transformation_table) &&
         WriteVector(file, sa.reverse_sym_operation) &&
         WriteVector(file, sa.conc_sym_operation) &&
         WriteVector(file, sa.m_over_n) &&
         WriteVector(file, sa.group_state_a_b) &&
         WriteVector(file, sa.group_state_c_d);
}

} /* namespace muehle */