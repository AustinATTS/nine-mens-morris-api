#ifndef MUEHLE_AI_MIN_MAX_AI_H_
#define MUEHLE_AI_MIN_MAX_AI_H_

#include <cstdio>
#include <vector>

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
    WarningArray
        warnings; /* Array containing the warnings for each field position */

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
    FieldClass field; /* Pointer to the current field [changed by Move()] */
    float current_value =
        0; /* Value of current situation for field->current_player */
    unsigned int cur_search_depth = 0;    /* Current level */
    std::vector<BackupStruct> old_states; /* For Undo() function */
  };

  /* Variables
   * 'mm' is the minimax algorithm instance
   * 'this' passes the current AI as the game interface
   * '100' sets the maximum search depth (chosen as a sape upper bound for
   * practical search limits). */
  mini_max::MiniMax mm{this, 100}; /* Minimax algorithm*/
  unsigned int depth_of_full_tree =
      0; /* Search depth where the whole tree is explored */
  mini_max::StateInfo info_about_choices;    /* Best move summary from the most
                                                recent Play() call */
  std::vector<ThreadVarsStruct> thread_vars; /* Information for each thread */

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
};

}  // namespace muehle

#endif  // MUEHLE_AI_MIN_MAX_AI_H_
