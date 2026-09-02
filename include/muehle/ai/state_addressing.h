#ifndef MUEHLE_AI_STATE_ADDRESSING_H_
#define MUEHLE_AI_STATE_ADDRESSING_H_

#include<filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

#include "muehle/field_struct.h"

namespace muehle {

/* All field states are divided into layers. Each layer number is determined by
 * the number of white and black stones on the field. Each square of the field
 * belongs to one of the four groups (A,B,C,D). CDCABACDCDBDDBDCDCABACDC Each
 * layer is divided into sublayers. The sublayer number is determined by the
 * number of white and black stones in group C and D. Each state within each
 * group (AB, CD) can be represented by a number (of type GroupStateNumber)
 * between 0 and 3^num_squares_group-1. The value is calculated by the sum of
 * 3^i * field[i]. When symmetry and the amount white/black stones are
 * considered, the number of states is reduced and can be indexed by the type
 * GroupIndex. */
class StateAddressing {
  friend class StateAddressingTest_internal_variables_Test;
  friend class StateAddressingTest_internal_functions_Test;
  friend class StateAddressingTest_totalNumMissingStones_Test;

 public:
  using GroupStateNumber =
      unsigned int; /* number of a state within a group (without considering
                       symmetry and the amount of white/black stones) */
  using GroupIndex =
      unsigned int; /* number of a state within a group (considering symmetry
                       and the amount of white/black stones) */
  using NumWhiteStones = unsigned int; /* number of white stones */
  using NumBlackStones = unsigned int; /* number of black stones */
  using SymOperationId = unsigned int; /* number of a symmetry operation */
  using SubLayerId = unsigned int;     /* number of a sublayer within a layer */
  using LayerId = unsigned int;        /* number of a layer */
  using StateId = unsigned int;        /* number of a state within a layer */

  /* The number of layers is calculated as follows:
   - 10 x 10 since each color can range from 0 to 9 stones
   - x2 since there is the setting phase and the moving phase */
  static const unsigned int NUM_LAYERS = 200;
  static const unsigned int MAX_NUM_SUB_LAYERS = 100;
  static const unsigned int LAYER_INDEX_MOVING_PHASE = 0;
  static const unsigned int LAYER_INDEX_SETTING_PHASE = 1;
  static constexpr unsigned int NOT_INDEXED =
      0xFFFFFFFFu; /* a constant that is used to indicate that a layer is not
                    indexed */
  static const unsigned int NUM_STONES_PER_PLAYER = 9;

  /* Symmetry Operations */
  static constexpr SymOperationId SO_TURN_LEFT = 0;
  static constexpr SymOperationId SO_TURN_180 = 1;
  static constexpr SymOperationId SO_TURN_RIGHT = 2;
  static constexpr SymOperationId SO_DO_NOTHING = 3;
  static constexpr SymOperationId SO_INVERT = 4;
  static constexpr SymOperationId SO_MIRROR_VERT = 5;
  static constexpr SymOperationId SO_MIRROR_HORI = 6;
  static constexpr SymOperationId SO_MIRROR_DIAG_1 = 7;
  static constexpr SymOperationId SO_MIRROR_DIAG_2 = 8;
  static constexpr SymOperationId SO_INV_LEFT = 9;
  static constexpr SymOperationId SO_INV_RIGHT = 10;
  static constexpr SymOperationId SO_INV_180 = 11;
  static constexpr SymOperationId SO_INV_MIR_VERT = 12;
  static constexpr SymOperationId SO_INV_MIR_HORI = 13;
  static constexpr SymOperationId SO_INV_MIR_DIAG_1 = 14;
  static constexpr SymOperationId SO_INV_MIR_DIAG_2 = 15;
  static constexpr SymOperationId NUM_SYM_OPERATIONS = 16;

 private:
  static constexpr unsigned int num_squares_group_a =
      4; /* number of stone fields in group A */
  static constexpr unsigned int num_squares_group_b = 4;
  static constexpr unsigned int num_squares_group_c = 8;
  static constexpr unsigned int num_squares_group_d = 8;
  static const unsigned int group_order_a = 7;
  static const unsigned int group_order_b = 3;
  static const unsigned int group_order_c = 15;
  static const unsigned int group_order_d = 7;
  static const unsigned int GROUP_A = 0; /* index of the group */
  static const unsigned int GROUP_B = 1;
  static const unsigned int GROUP_C = 2;
  static const unsigned int GROUP_D = 3;
  static const unsigned int MAX_NUM_SITUATIONS_A =
      81; /* 3^num_squares_group_a; */
  static const unsigned int MAX_NUM_SITUATIONS_B =
      81; /* 3^num_squares_group_b; */
  static const unsigned int MAX_NUM_SITUATIONS_C =
      81 * 81; /* 3^num_squares_group_c; */
  static const unsigned int MAX_NUM_SITUATIONS_D =
      81 * 81; /* 3^num_squares_group_d; */

  /* Define the four groups */
  static constexpr unsigned int square_index_group_a[] = {3, 5, 20, 18};
  static constexpr unsigned int square_index_group_b[] = {4, 13, 19, 10};
  static constexpr unsigned int square_index_group_c[] = {0, 2, 23, 21,
                                                          6, 8, 17, 15};
  static constexpr unsigned int square_index_group_d[] = {1,  7,  14, 12,
                                                          22, 16, 9,  11};
  static constexpr unsigned int field_pos_is_of_group[] = {
      GROUP_C, GROUP_D, GROUP_C, GROUP_A, GROUP_B, GROUP_A, GROUP_C, GROUP_D,
      GROUP_C, GROUP_D, GROUP_B, GROUP_D, GROUP_D, GROUP_B, GROUP_D, GROUP_C,
      GROUP_D, GROUP_C, GROUP_A, GROUP_B, GROUP_A, GROUP_C, GROUP_D, GROUP_C};

  static constexpr unsigned int so_table_turn_left[] = {
      2,  14, 23, 5, 13, 20, 8, 12, 17, 1, 4, 7,
      16, 19, 22, 6, 11, 15, 3, 10, 18, 0, 9, 21};

  static constexpr unsigned int so_table_do_nothing[] = {
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11,
      12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23};

  static constexpr unsigned int so_table_mirror_hori[] = {
      21, 22, 23, 18, 19, 20, 15, 16, 17, 9, 10, 11,
      12, 13, 14, 6,  7,  8,  3,  4,  5,  0, 1,  2};

  static constexpr unsigned int so_table_turn_180[] = {
      23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12,
      11, 10, 9,  8,  7,  6,  5,  4,  3,  2,  1,  0};

  static constexpr unsigned int so_table_invert[] = {
      6,  7,  8,  3,  4,  5,  0,  1,  2,  11, 10, 9,
      14, 13, 12, 21, 22, 23, 18, 19, 20, 15, 16, 17};

  static constexpr unsigned int so_table_inv_mir_hori[] = {
      15, 16, 17, 18, 19, 20, 21, 22, 23, 11, 10, 9,
      14, 13, 12, 0,  1,  2,  3,  4,  5,  6,  7,  8};

  static constexpr unsigned int so_table_inv_mir_vert[] = {
      8, 7,  6,  5,  4,  3,  2,  1,  0,  12, 13, 14,
      9, 10, 11, 23, 22, 21, 20, 19, 18, 17, 16, 15};

  static constexpr unsigned int so_table_inv_mir_diag_1[] = {
      17, 12, 8, 20, 13, 5, 23, 14, 2, 16, 19, 22,
      1,  4,  7, 21, 9,  0, 18, 10, 3, 15, 11, 6};

  static constexpr unsigned int so_table_inv_mir_diag_2[] = {
      6,  11, 15, 3, 10, 18, 0, 9,  21, 7, 4,  1,
      22, 19, 16, 2, 14, 23, 5, 13, 20, 8, 12, 17};

  static constexpr unsigned int so_table_inv_left[] = {
      8,  12, 17, 5, 13, 20, 2, 14, 23, 7, 4,  1,
      22, 19, 16, 0, 9,  21, 3, 10, 18, 6, 11, 15};

  static constexpr unsigned int so_table_inv_right[] = {
      15, 11, 6, 18, 10, 3, 21, 9,  0, 16, 19, 22,
      1,  4,  7, 23, 14, 2, 20, 13, 5, 17, 12, 8};

  static constexpr unsigned int so_table_inv_180[] = {
      17, 16, 15, 20, 19, 18, 23, 22, 21, 12, 13, 14,
      9,  10, 11, 2,  1,  0,  5,  4,  3,  8,  7,  6};

  static constexpr unsigned int so_table_mirror_diag_1[] = {
      0,  9,  21, 3, 10, 18, 6, 11, 15, 1, 4,  7,
      16, 19, 22, 8, 12, 17, 5, 13, 20, 2, 14, 23};

  static constexpr unsigned int so_table_turn_right[] = {
      21, 9, 0, 18, 10, 3, 15, 11, 6, 22, 19, 16,
      7,  4, 1, 17, 12, 8, 20, 13, 5, 23, 14, 2};

  static constexpr unsigned int so_table_mirror_vert[] = {
      2,  1,  0, 5,  4,  3,  8,  7,  6,  14, 13, 12,
      11, 10, 9, 17, 16, 15, 20, 19, 18, 23, 22, 21};

  static constexpr unsigned int so_table_mirror_diag_2[] = {
      23, 14, 2, 20, 13, 5, 17, 12, 8, 22, 19, 16,
      7,  4,  1, 15, 11, 6, 18, 10, 3, 21, 9,  0};

  /* structs */
  /* each layer is divided into sublayers, based on the number of white/black
   * stones in group C and D */
  struct SubLayerStruct {
    GroupIndex min_ndex;  /* index of the first state of this sublayer in the
                             database */
    GroupIndex max_index; /* index of the last  state of this sublayer in the
                             database */
    NumWhiteStones num_white_stones_group_c_d; /* number of white stones in
                                                  group C and D */
    NumBlackStones num_black_stones_group_c_d; /* number of black stones in
                                                  group C and D */
    NumWhiteStones num_white_stones_group_a_b; /* number of white stones in
                                                  group A and B */
    NumBlackStones num_black_stones_group_a_b; /* number of black stones in
                                                  group A and B */
  };

  /* layer */
  struct LayerStruct {
    NumWhiteStones amount_white_stones; /* number of white stones */
    NumBlackStones amount_black_stones; /* number of black stones */
    SubLayerId num_sub_layers;          /* number of sublayers */
    SubLayerId sub_layer_index_a_b[NUM_STONES_PER_PLAYER + 1]
                                  [NUM_STONES_PER_PLAYER + 1]; /* unused */
    SubLayerId sub_layer_index_c_d[NUM_STONES_PER_PLAYER + 1]
                                  [NUM_STONES_PER_PLAYER +
                                   1]; /* mapping [number of white stones in
                                       group CD][number of black stones in group
                                       CD] to index within subLayer[] */
    SubLayerStruct sub_layer[MAX_NUM_SUB_LAYERS]; /* sublayers */

    unsigned int GetStateNumberWithInSubLayer(StateId state_number,
                                              bool setting_phase) const;
    void GetNumGroupStonesByStateNumber(
        StateId state_number, bool setting_phase,
        NumWhiteStones& num_white_stones_group_a_b,
        NumBlackStones& num_black_stones_group_a_b,
        NumWhiteStones& num_white_stones_group_c_d,
        NumBlackStones& num_black_stones_group_c_d) const;
  };

  /* 1d, 2d, 3d vector types */
  template <typename T>
  using Vector1D = std::vector<T>;

  template <typename T>
  using Vector2D = std::vector<std::vector<T>>;

  template <typename T>
  using Vector3D = std::vector<std::vector<std::vector<T>>>;

  template <typename T>
  static void ResizeVector1D(Vector1D<T>& vec, T value, size_t size) {
    vec.resize(size, value);
  }

  template <typename T>
  static void ResizeVector2D(Vector2D<T>& vec, T value, size_t x, size_t y) {
    vec.resize(x, Vector1D<T>(y, value));
  }

  template <typename T>
  static void ResizeVector3D(Vector3D<T>& vec, T value, size_t x, size_t y,
                             size_t z) {
    vec.resize(x, Vector2D<T>(y, Vector1D<T>(z, value)));
  }

  /* Since the calculation of the variables takes some time, they are cached in
  the file pre_calced_vars.dat */
  class CacheFile {
   private:
    struct FileHeaderStruct {
      unsigned int size_in_bytes = 0;
    };

    template <typename T>
    static bool WriteVector(std::fstream& file, const Vector1D<T>& vec) {
      file.write(reinterpret_cast<const char*>(vec.data()),
                 sizeof(T) * vec.size());
      return file.good();
    }

    template <typename T>
    static bool WriteVector(std::fstream& file, const Vector2D<T>& vec) {
      for (const auto& sub_vec : vec) {
        if (!WriteVector(file, sub_vec)) return false;
      }
      return true;
    }

    template <typename T>
    static bool WriteVector(std::fstream& file, const Vector3D<T>& vec) {
      for (const auto& sub_vec_2d : vec) {
        for (const auto& sub_vec : sub_vec_2d) {
          if (!WriteVector(file, sub_vec)) return false;
        }
      }
      return true;
    }

    template <typename T>
    static bool ReadVector(std::fstream& file, Vector1D<T>& vec) {
      file.read(reinterpret_cast<char*>(vec.data()), sizeof(T) * vec.size());
      return file.good();
    }

    template <typename T>
    static bool ReadVector(std::fstream& file, Vector2D<T>& vec) {
      for (auto& sub_vec : vec) {
        if (!ReadVector(file, sub_vec)) return false;
      }
      return true;
    }

    template <typename T>
    static bool ReadVector(std::fstream& file, Vector3D<T>& vec) {
      for (auto& sub_vec_2d : vec) {
        for (auto& sub_vec : sub_vec_2d) {
          if (!ReadVector(file, sub_vec)) return false;
        }
      }
      return true;
    }

    /* internal variables */
    std::fstream file;
    std::wstring file_path;
    FileHeaderStruct header;
    StateAddressing& sa;

   public:
    CacheFile(std::wstring const& directory, StateAddressing& sa);
    ~CacheFile();

    bool ReadFromFile();
    bool WriteToFile();
  };

  /* mathematical functions */
  static long long MOverNFunction(unsigned int m, unsigned int n);

  /* symmetry functions */
  void ApplySymmetryTransfToField(SymOperationId symmetry_operation_number,
                                  bool do_inverse_operation,
                                  const FieldStruct::FieldArray& source_field,
                                  FieldStruct::FieldArray& dest_field) const;
  void ApplySymmetryTransfToField(SymOperationId symmetry_operation_number,
                                  bool do_inverse_operation,
                                  const FieldStruct::MillArray& source_field,
                                  FieldStruct::MillArray& dest_field) const;

  /* helper functions */
  void AdaptFieldArrayToCurPlayer(const FieldStruct::FieldArray& src_field,
                                  FieldStruct::FieldArray& dst_field,
                                  PlayerId cur_player) const;
  void CountStonesInGroup(const FieldStruct::Core& field,
                          NumWhiteStones& num_white_stones_group_a_b,
                          NumBlackStones& num_black_stones_group_a_b,
                          NumWhiteStones& num_white_stones_group_c_d,
                          NumBlackStones& num_black_stones_group_c_d) const;
  void CountStonesInGroupCD(const FieldStruct::Core& field,
                            NumWhiteStones& num_white_stones_group_c_d,
                            NumBlackStones& num_black_stones_group_c_d) const;
  bool AddTotalNumMissingStonesOffset(StateId& state_number,
                                      const FieldStruct::Core& field) const;
  bool IsSettingPhase(LayerId layer_num) const;
  static unsigned int GetMaxTotalNumMissingStones(
      NumWhiteStones amount_white_stones, NumBlackStones amount_black_stones);
  unsigned int GetTotalNumMissingStones(
      StateId state_number, bool setting_phase,
      NumWhiteStones amount_white_stones,
      NumBlackStones amount_black_stones) const;

  /* init functions */
  void InitMOverN();
  void InitPowerOfThree();
  void InitSymOperationMappings();
  void InitConcSymOperation();
  void InitGroupAB();
  void InitGroupCD();
  void InitLayerRegardingSettingPhase();
  void InitLayerRegardingMovingPhase();
  static inline void CalcFieldBasedOnGroup(
      FieldStruct::FieldArray& field, unsigned int num_squares_in_group,
      GroupStateNumber state, const unsigned int* square_index_group,
      unsigned int groupOrder, const Vector1D<unsigned int>& power_of_three);
  void CalcFieldBasedOnGroupAB(FieldStruct::FieldArray& field,
                               GroupStateNumber state_a_b) const;
  void CalcFieldBasedOnGroupCD(FieldStruct::FieldArray& field,
                               GroupStateNumber state_c_d) const;
  static inline void CalcGroupStateNumberBasedOnField(
      const FieldStruct::FieldArray& field, unsigned int num_squares_in_group,
      GroupStateNumber& state_number, const unsigned int* square_index_group,
      unsigned int group_order, const Vector1D<unsigned int>& power_of_three);
  void CalcGroupStateNumberAB(const FieldStruct::FieldArray& field,
                              GroupStateNumber& state_number_a_b) const;
  void CalcGroupStateNumberCD(const FieldStruct::FieldArray& field,
                              GroupStateNumber& state_number_c_d) const;
  void ResizeGroupStateMappingArray(
      Vector3D<unsigned int>& original_state,
      const Vector2D<unsigned int>* p_amount_situations,
      unsigned int num_squares_in_group) const;

  /* internal variables */
  Vector1D<GroupIndex> group_index_a_b; /* mapping [GroupStateNumber] to
                                        GroupIndex within group AB */
  Vector1D<GroupIndex> group_index_c_d; /* mapping [GroupStateNumber] to
                                        GroupIndex within group CD */
  Vector3D<GroupStateNumber>
      group_state_a_b; /* mapping [number of white stones][number of black
                       stones][GroupIndex] to GroupStateNumber with in group
                       AB */
  Vector3D<GroupStateNumber>
      group_state_c_d; /* mapping [number of white stones][number of black
                       stones][GroupIndex] to GroupStateNumber with in group
                       CD */
  Vector2D<GroupIndex>
      amount_situations_a_b; /* mapping [number of white stones][number of
                             black stones] to number of situations for group
                             A and B (considering symmetry operations). this
                             corresponds to the maximum GroupIndex within
                             group AB */
  Vector2D<GroupIndex>
      amount_situations_c_d; /* mapping [number of white stones][number of
                             black stones] to number of situations for group
                             C and D (considering symmetry operations). this
                             corresponds to the maximum GroupIndex within
                             group CD */
  Vector1D<SymOperationId>
      symmetry_operation_c_d; /* index of symmetry operation used to get from
                              the symmetric state to one listed in
                              group_index_c_d */
  Vector1D<unsigned int> power_of_three; /* 3^0, 3^1, 3^2, ... */
  Vector2D<unsigned int> m_over_n;       /* mapping [m][n] to m over n */
  Vector1D<SymOperationId>
      reverse_sym_operation; /* index of the reverse symmetry operation:
                             [symmetry operation] -> reverse symmetry
                             operation */
  Vector2D<unsigned int>
      symmetry_transformation_table; /* matrix used for application of the
                                      symmetry operations to the field:
                                      [symmetry operation][field position] */
  Vector3D<unsigned int>
      layer_index;             /* mapping [moving/setting phase][number of white
                               stones][number of black stones] to layer index */
  Vector1D<LayerStruct> layer; /* information about the layers */

 public:
  Vector2D<SymOperationId>
      conc_sym_operation; /* symmetry operation, which is identical to applying
                             those two concatenated symmetry operations:
                             [symmetry operation 1][symmetry operation 2] ->
                             resulting symmetry operation */

  /* constructor */
  StateAddressing(std::wstring const& directory);

  /* getter */
  const LayerStruct& GetLayer(LayerId layer_num) const;
  unsigned int GetNumberOfKnotsInLayer(LayerId layer_num) const;
  unsigned int GetLayerNumber(unsigned int num_stones_of_cur_player,
                              unsigned int num_stones_of_opp_player,
                              bool is_setting_phase) const;
  unsigned int GetLayerNumber(const FieldStruct::Core& field) const;
  bool GetStateNumber(LayerId layer_num, StateId& state_number,
                      SymOperationId& sym_op,
                      const FieldStruct::Core& field) const;
  bool GetFieldByStateNumber(LayerId layer_num, StateId state_number,
                             FieldStruct& field, PlayerId cur_player) const;

  /* symmetry functions */
  bool ApplySymmetryTransfToField(SymOperationId symmetry_operation_number,
                                  bool do_inverse_operation,
                                  FieldStruct& field) const;
  bool ApplySymmetryTransfToField(SymOperationId symmetry_operation_number,
                                  bool do_inverse_operation,
                                  FieldStruct::Core& field) const;
  bool GetStateNumbersOfSymmetricStates(
      const FieldStruct::Core& field,
      std::array<StateId, StateAddressing::NUM_SYM_OPERATIONS>& state_numbers)
      const;
  bool IsSymOperationInvariant(SymOperationId symmetry_operation,
                               const FieldStruct::Core& field) const;
};

}  // namespace muehle

#endif  // MUEHLE_AI_STATE_ADDRESSING_H_
