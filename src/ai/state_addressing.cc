#include "muehle/ai/state_addressing.h"

namespace muehle {

// Implementation goes here.

unsigned int StateAddressing::LayerStruct::GetStateNumberWithInSubLayer(
    StateId state_number, bool setting_phase) const {
}

void StateAddressing::LayerStruct::GetNumGroupStonesByStateNumber(
    StateId state_number, bool setting_phase,
    NumWhiteStones& num_white_stones_group_a_b,
    NumBlackStones& num_black_stones_group_a_b,
    NumWhiteStones& num_white_stones_group_c_d,
    NumBlackStones& num_black_stones_group_c_d) const {
}

StateAddressing::CacheFile::CacheFile(std::wstring const& directory,
    StateAddressing& sa) {
}

StateAddressing::CacheFile::~CacheFile() {
}

bool StateAddressing::CacheFile::ReadFromFile() {
}

bool StateAddressing::CacheFile::WriteToFile() {
}

long long StateAddressing::MOverNFunction(unsigned int m, unsigned int n) {
}

void StateAddressing::ApplySymmetryTransfToField(
    SymOperationId symmetry_operation_number, bool do_inverse_operation,
    const FieldStruct::FieldArray& source_field,
    FieldStruct::FieldArray& dest_field) const {
}

void StateAddressing::ApplySymmetryTransfToField(
    SymOperationId symmetry_operation_number, bool do_inverse_operation,
    const FieldStruct::MillArray& source_field,
    FieldStruct::MillArray& dest_field) const {
}

void StateAddressing::AdaptFieldArrayToCurPlayer(
    const FieldStruct::FieldArray& src_field,
    FieldStruct::FieldArray& dst_field, PlayerId cur_player) const {
}

void StateAddressing::CountStonesInGroup(const FieldStruct::Core& field,
    NumWhiteStones& num_white_stones_group_a_b,
    NumBlackStones& num_black_stones_group_a_b,
    NumWhiteStones& num_white_stones_group_c_d,
    NumBlackStones& num_black_stones_group_c_d) const {
}

void StateAddressing::CountStonesInGroupCD(const FieldStruct::Core& field,
    NumWhiteStones& num_white_stones_group_c_d,
    NumBlackStones& num_black_stones_group_c_d) const {
}

bool StateAddressing::AddTotalNumMissingStonesOffset(StateId& state_number,
    const FieldStruct::Core& field) const {
}

bool StateAddressing::IsSettingPhase(LayerId layer_num) const {
}

unsigned int StateAddressing::GetMaxTotalNumMissingStones(
    NumWhiteStones amount_white_stones, NumBlackStones amount_black_stones) {
}

unsigned int StateAddressing::GetTotalNumMissingStones(StateId state_number,
    bool setting_phase, NumWhiteStones amount_white_stones,
    NumBlackStones amount_black_stones) const {
}

void StateAddressing::InitMOverN() {
}

void StateAddressing::InitPowerOfThree() {
}

void StateAddressing::InitSymOperationMappings() {
}

void StateAddressing::InitConcSymOperation() {
}

void StateAddressing::InitGroupAB() {
}

void StateAddressing::InitGroupCD() {
}

void StateAddressing::InitLayerRegardingSettingPhase() {
}

void StateAddressing::InitLayerRegardingMovingPhase() {
}

void StateAddressing::CalcFieldBasedOnGroup(FieldStruct::FieldArray& field,
    unsigned int num_squares_in_group, GroupStateNumber state,
    const unsigned int* square_index_group, unsigned int groupOrder,
    const Vector1D<unsigned int>& power_of_three) {
}

void StateAddressing::CalcFieldBasedOnGroupAB(FieldStruct::FieldArray& field,
    GroupStateNumber state_a_b) const {
}

void StateAddressing::CalcFieldBasedOnGroupCD(FieldStruct::FieldArray& field,
    GroupStateNumber state_c_d) const {
}

void StateAddressing::CalcGroupStateNumberBasedOnField(
    const FieldStruct::FieldArray& field, unsigned int num_squares_in_group,
    GroupStateNumber& state_number, const unsigned int* square_index_group,
    unsigned int group_order, const Vector1D<unsigned int>& power_of_three) {
}

void StateAddressing::CalcGroupStateNumberAB(
    const FieldStruct::FieldArray& field,
    GroupStateNumber& state_number_a_b) const {
}

void StateAddressing::CalcGroupStateNumberCD(
    const FieldStruct::FieldArray& field,
    GroupStateNumber& state_number_c_d) const {
}

void StateAddressing::ResizeGroupStateMappingArray(
    Vector3D<unsigned int>& original_state,
    const Vector2D<unsigned int>* p_amount_situations,
    unsigned int num_squares_in_group) const {
}

StateAddressing::StateAddressing(std::wstring const& directory) {
}

const StateAddressing::LayerStruct& StateAddressing::GetLayer(
    LayerId layer_num) const {
}

unsigned int StateAddressing::GetNumberOfKnotsInLayer(LayerId layer_num) const {
}

unsigned int StateAddressing::GetLayerNumber(
    unsigned int num_stones_of_cur_player,
    unsigned int num_stones_of_opp_player, bool is_setting_phase) const {
}

unsigned int StateAddressing::GetLayerNumber(
    const FieldStruct::Core& field) const {
}

bool StateAddressing::GetStateNumber(LayerId layer_num, StateId& state_number,
    SymOperationId& sym_op, const FieldStruct::Core& field) const {
}

bool StateAddressing::GetFieldByStateNumber(LayerId layer_num,
    StateId state_number, FieldStruct& field, PlayerId cur_player) const {
}

bool StateAddressing::ApplySymmetryTransfToField(
    SymOperationId symmetry_operation_number, bool do_inverse_operation,
    FieldStruct& field) const {
}

bool StateAddressing::ApplySymmetryTransfToField(
    SymOperationId symmetry_operation_number, bool do_inverse_operation,
    FieldStruct::Core& field) const {
}

bool StateAddressing::GetStateNumbersOfSymmetricStates(
    const FieldStruct::Core& field,
    std::array<StateId, StateAddressing::NUM_SYM_OPERATIONS>& state_numbers)
const {
}

bool StateAddressing::IsSymOperationInvariant(SymOperationId symmetry_operation,
    const FieldStruct::Core& field) const {
}
}  // namespace muehle
