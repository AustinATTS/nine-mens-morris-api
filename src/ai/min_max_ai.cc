#include "muehle/ai/min_max_ai.h"

namespace muehle {

MinMaxAi::FieldClass::FieldClass() : FieldStruct() {
  warnings.fill(WarningId::no_warning);
}

MinMaxAi::FieldClass::FieldClass(const FieldStruct& the_field)
    : FieldStruct(the_field) {
  warnings.fill(WarningId::no_warning);

  /* Go in every direction */
  for (unsigned int i = 0; i < size; i++) {
    SetWarningAndMill(i, neighbour[i][0][0], neighbour[i][0][1]);
    SetWarningAndMill(i, neighbour[i][1][0], neighbour[i][1][1]);
  }
}

void MinMaxAi::FieldClass::SetWarningAndMill(unsigned int stone,
                                             unsigned int first_neighbour,
                                             unsigned int second_neighbour) {
  PlayerId row_owner = field[stone];
  WarningId row_owner_warning = (row_owner == PlayerId::player_one)
                                    ? WarningId::player_one_warning
                                    : WarningId::player_two_warning;

  if (row_owner != PlayerId::square_is_free &&
      field[first_neighbour] == PlayerId::square_is_free &&
      field[second_neighbour] == row_owner) {
    warnings[first_neighbour] =
        AddWarning(warnings[first_neighbour], row_owner_warning);
  }
  if (row_owner != PlayerId::square_is_free &&
      field[first_neighbour] == row_owner) {
    warnings[second_neighbour] =
        AddWarning(warnings[second_neighbour], row_owner_warning);
  }
}

WarningId MinMaxAi::FieldClass::AddWarning(WarningId existing_warning,
                                           WarningId new_warning) {
  if (existing_warning == WarningId::no_warning) {
    return new_warning;
  }
  if (new_warning == WarningId::no_warning) {
    return existing_warning;
  }
  return static_cast<WarningId>(static_cast<unsigned int>(existing_warning) |
                                static_cast<unsigned int>(new_warning));
}

void MinMaxAi::PrepareCalculation() {
  /* Calculate current value */
  for (auto& vars : thread_vars) {
    vars.current_value = 0;
  }
}

void MinMaxAi::GetPossibilities(unsigned int thread_no,
                                std::vector<unsigned int>& possibility_ids) {
  thread_vars[thread_no].field.GetPossibilities(possibility_ids);
}

unsigned int MinMaxAi::GetMaxNumPossibilities() {
  return FieldStruct::max_num_pos_moves;
}

void MinMaxAi::GetValueOfSituation(unsigned int thread_no, float& float_value,
                                   mini_max::TwoBit& short_value) {
  FieldClass& field = thread_vars[thread_no].field;
  float& current_value = thread_vars[thread_no].current_value;

  if (field.HasGameFinished()) {
    current_value = (field.GetWinner() == field.GetCurPlayer().id)
                        ? mini_max::FPKV_MAX_VALUE
                        : mini_max::FPKV_MIN_VALUE;
    short_value = (field.GetWinner() == field.GetCurPlayer().id)
                      ? mini_max::SKV_VALUE_GAME_WON
                      : mini_max::SKV_VALUE_GAME_LOST;
  } else {
    int opp_missing = field.GetOppPlayer().num_stones_missing;
    int cur_missing = field.GetCurPlayer().num_stones_missing;
    float cur_moves = field.GetCurPlayer().num_possible_moves * 0.1f;
    float opp_moves = field.GetOppPlayer().num_possible_moves * 0.1f;
    current_value =
        static_cast<float>(opp_missing - cur_missing) + cur_moves - opp_moves;
    short_value = mini_max::SKV_VALUE_GAME_DRAWN;
  }
  float_value = current_value;
}

void MinMaxAi::Move(unsigned int thread_no, unsigned int id_possibility,
                    bool& player_to_move_changed, void*& p_backup) {
  /* Locals */
  FieldClass& field = thread_vars[thread_no].field;
  float& current_value = thread_vars[thread_no].current_value;
  unsigned int& cur_search_depth = thread_vars[thread_no].cur_search_depth;
  std::vector<BackupStruct>& old_states = thread_vars[thread_no].old_states;
  MinMaxAi::BackupStruct& old_state_mm = old_states[cur_search_depth];
  FieldStruct::BackupStruct& old_state_fs = old_state_mm;
  MoveInfo move_to_make;

  /* Convert possibility ID to MoveInfo */
  move_to_make.SetId(id_possibility);

  /* Backup */
  old_state_mm.value = current_value;
  p_backup = (void*)&old_states[cur_search_depth];
  cur_search_depth++;

  if (!field.Move(move_to_make, old_state_fs)) {
    throw std::runtime_error(
        "Invalid move detected during MinMaxAi::Move execution.");
  };

  /* Player changes after every move */
  player_to_move_changed = true;
}

void MinMaxAi::Undo(unsigned int thread_no, unsigned int id_possibility,
                    bool& player_to_move_changed, void* p_backup) {
  /* Locals */
  FieldClass& field = thread_vars[thread_no].field;
  float& current_value = thread_vars[thread_no].current_value;
  unsigned int& cur_search_depth = thread_vars[thread_no].cur_search_depth;
  std::vector<BackupStruct>& old_states = thread_vars[thread_no].old_states;
  if (p_backup == nullptr) {
    throw std::runtime_error(
        "Null backup pointer detected during MiniMaxAI::Undo execution.");
  }
  BackupStruct& old_state_mm = *static_cast<BackupStruct*>(p_backup);

  /* Reset old value */
  current_value = old_state_mm.value;
  cur_search_depth--;

  FieldStruct::BackupStruct& old_state_fs = old_state_mm;
  if (!field.Undo(old_state_fs)) {
    throw std::runtime_error(
        "Invalid Undo detected during MiniMaxAi::Undo execution.");
  }

  /* Player always changes back after undo */
  player_to_move_changed = true;
}

void MinMaxAi::PrintMoveInformation(unsigned int thread_no,
                                    unsigned int id_possibility) {
  /* Convert possibility ID to MoveInfo to print move details */
  MoveInfo move;
  move.SetId(id_possibility);

  /* Move */
  if (thread_vars[thread_no].field.InSettingPhase()) {
    std::cout << "Set stone to " << (char)(move.to + 97) << std::endl;
  } else {
    std::cout << "Move from " << (char)(move.from + 97) << " to "
              << (char)(move.to + 97) << std::endl;
  }
  if (move.remove_stone < FieldStruct::size) {
    std::cout << "Remove stone from " << (char)(move.remove_stone + 97)
              << std::endl;
  }
}

/* MinMaxAi Class Constructor */
MinMaxAi::MinMaxAi() : state_addressing(L".") {
  thread_vars.resize(mm.GetNumThreads(), ThreadVarsStruct());
}

/* MinMaxAI class destructor */
MinMaxAi::~MinMaxAi() {}

/* Play */
void MinMaxAi::Play(const FieldStruct& the_field, MoveInfo& move) {
  /* Globals */
  for (auto& vars : thread_vars) {
    vars.field = the_field;
    vars.cur_search_depth = 0;
    vars.current_value = 0;
    vars.old_states.clear();
  }
  unsigned int best_choice;
  unsigned int search_depth;

  /* Automatic depth */
  if (depth_of_full_tree == 0) {
    if (the_field.InSettingPhase()) {
      search_depth = 5;
    } else if (the_field.GetCurPlayer().num_stones <= 4) {
      search_depth = 7;
    } else if (the_field.GetOppPlayer().num_stones <= 4) {
      search_depth = 7;
    } else {
      search_depth = 7;
    }
  } else {
    search_depth = depth_of_full_tree;
  }

  /* Reserve memory */
  for (auto& vars : thread_vars) {
    vars.old_states.resize(search_depth + 3);
  }

  /* Start the MiniMax algorithm */
  mm.SetSearchDepth(search_depth);
  if (!mm.GetBestChoice(best_choice, info_about_choices)) {
    throw std::runtime_error(
        "Error in MinMaxAi::Play() - GetBestChoice() failed.");
  }

  /* Decode the best choice. Convert possibility ID to MoveInfo with integrated
   * stone removal */
  move.SetId(best_choice);
}

/* Sets the search depth */
void MinMaxAi::SetSearchDepth(unsigned int depth) {
  depth_of_full_tree = depth;
}

const mini_max::StateInfo& MinMaxAi::GetInfoAboutChoices() const {
  return info_about_choices;
}

unsigned int MinMaxAi::GetLayerNumber(unsigned int thread_no) {
  return state_addressing.GetLayerNumber(thread_vars[thread_no].field);
}

void MinMaxAi::GetLayerAndStateNumber (unsigned int thread_no, unsigned int& layer_num, unsigned int& state_num, unsigned int& sym_op) {
  layer_num = state_addressing.GetLayerNumber(thread_vars[thread_no].field);
  state_addressing.GetStateNumber(layer_num, state_num, sym_op, thread_vars[thread_no].field);
}

}  // namespace muehle