#ifndef MUEHLE_AI_MIN_MAX_AI_H_
#define MUEHLE_AI_MIN_MAX_AI_H_

#include <cstdio>
#include <vector>

#include "muehle/ai/state_addressing.h"
#include "muehle/field_struct.h"
#include "muehle/mini_max/mini_max.h"
#include "muehle/muehle.h"

namespace muehle {

/* AI player using the MiniMax algorithm for the Muehle game.
 * This class implements an AI player that utilises the MiniMax algorithm to
 * determine optimal moves in the Muehle game. It interfaces with the MiniMax
 * library and provides methods for move calculation, evaluation, and game state
 * management. */
class MinMaxAi : public MuehleAI, mini_max::GameInterface {
 protected:
  using WarningArray = std::array<WarningId, FieldStruct::size>;

  /* Classes */
  class FieldClass : public FieldStruct {
   public:
    /* Array containing the warnings for each field position */
    WarningArray warnings;

    FieldClass();
    FieldClass(const FieldStruct& the_field);

   private:
    void SetWarningAndMill(unsigned int stone, unsigned int first_neighbour,
                           unsigned int second_neighbour);
    static WarningId AddWarning(WarningId existing_warning,
                                WarningId new_warning);
  };

  struct BackupStruct : public FieldStruct::BackupStruct {
    float value;
    WarningArray warnings;
  };

  struct ThreadVarsStruct {
    /* Pointer to the current field [changed by Move()] */
    FieldClass field;

    /* Value of current situation for field->current_player */
    float current_value = 0;

    /* Current level */
    unsigned int cur_search_depth = 0;

    /* For Undo() function */
    std::vector<BackupStruct> old_states;
  };

  /* Variables */

  /* 'mm' is the minimax algorithm instance
   * 'this' passes the current AI as the game interface
   * '100' sets the maximum search depth (chosen as a sape upper bound for
   * practical search limits). Minimax algorithm */
  mini_max::MiniMax mm{this, 100};

  /* Search depth where the whole tree is explored */
  unsigned int depth_of_full_tree = 0;

  /* Best move summary from the most recent Play() call */
  mini_max::StateInfo info_about_choices;

  /* Information for each thread */
  std::vector<ThreadVarsStruct> thread_vars;

  /* Init */
  void PrepareCalculation() override;

  /* Getter */
  void GetPossibilities(unsigned int thread_no,
                        std::vector<unsigned int>& possibility_ids) override;
  unsigned int GetMaxNumPossibilities() override;
  void GetValueOfSituation(unsigned int thread_no, float& float_value,
                           mini_max::TwoBit& short_value) override;

  /* Setter */
  void Move(unsigned int thread_no, unsigned int id_possibility,
            bool& player_to_move_changed, void*& p_backup) override;
  void Undo(unsigned int thread_no, unsigned int id_possibility,
            bool& player_to_move_changed, void* p_backup) override;

  /* Output */
  void PrintField(unsigned int thread_no, mini_max::TwoBit value,
                  unsigned int indent_spaces) override {};
  void PrintMoveInformation(unsigned int thread_no,
                            unsigned int id_possibility) override;
  std::wstring GetOutputInformation(unsigned int layer_num) override {
    return std::wstring(L"");
  };

 public:
  /* Constructor / Destructor */
  MinMaxAi();
  ~MinMaxAi();

  /* Functions */
  void Play(const FieldStruct& the_field, MoveInfo& move) override;
  void SetSearchDepth(unsigned int depth);
  const mini_max::StateInfo& GetInfoAboutChoices() const;

  bool OpenDatabase(const std::wstring& directory,
                    bool use_comp_file_if_both_exist = true) {
    return mm.OpenDatabase(directory, use_comp_file_if_both_exist);
  }

  StateAddressing state_addressing;

  unsigned int GetLayerNumber(unsigned int thead_no) override;

  void GetLayerAndStateNumber(unsigned int thread_no, unsigned int& layer_num,
                              unsigned int& state_number,
                              unsigned int& sym_op) override;
};

} /* namespace muehle */

#endif /* MUEHLE_AI_MIN_MAX_AI_H_ */
