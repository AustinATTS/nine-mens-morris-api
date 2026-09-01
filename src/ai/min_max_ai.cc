#include "muehle/ai/min_max_ai.h"

namespace muehle {

// Implementation goes here.

MinMaxAi::FieldClass::FieldClass() {
}

MinMaxAi::FieldClass::FieldClass(const FieldStruct& the_field) {
}

void MinMaxAi::FieldClass::SetWarningAndMill(unsigned int stone,
    unsigned int first_neighbour, unsigned int second_neighbour) {
}

WarningId MinMaxAi::FieldClass::AddWarning(WarningId existing_warning,
    WarningId new_warning) {
}

void MinMaxAi::PrepareCalculation() {
  GameInterface::PrepareCalculation();
}

void MinMaxAi::GetPossibilities(unsigned int thread_no,
    std::vector<unsigned int>& possibility_ids) {
  GameInterface::GetPossibilities(thread_no, possibility_ids);
}

unsigned int MinMaxAi::GetMaxNumPossibilities() {
  return GameInterface::GetMaxNumPossibilities();
}

void MinMaxAi::GetValueOfSituation(unsigned int thread_no, float& float_value,
    mini_max::TwoBit& short_value) {
  GameInterface::GetValueOfSituation(thread_no, float_value, short_value);
}

void MinMaxAi::Move(unsigned int thread_no, unsigned int id_possibility,
    bool& player_to_move_changed, void*& p_backup) {
  GameInterface::Move(thread_no, id_possibility, player_to_move_changed,
                      p_backup);
}

void MinMaxAi::Undo(unsigned int thread_no, unsigned int id_possibility,
    bool& player_to_move_changed, void* p_backup) {
}

void MinMaxAi::PrintMoveInformation(unsigned int thread_no,
    unsigned int id_possibility) {
  GameInterface::PrintMoveInformation(thread_no, id_possibility);
}

MinMaxAi::MinMaxAi() {
}

MinMaxAi::~MinMaxAi() {
}

void MinMaxAi::Play(const FieldStruct& the_field, MoveInfo& move) {
}

void MinMaxAi::SetSearchDepth(unsigned int depth) {
}

const mini_max::StateInfo& MinMaxAi::GetInfoAboutChoices() const {
}
}  // namespace muehle
