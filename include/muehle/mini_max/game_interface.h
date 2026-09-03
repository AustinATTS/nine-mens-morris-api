#ifndef MUEHLE_MINI_MAX_GAME_INTERFACE_H_
#define MUEHLE_MINI_MAX_GAME_INTERFACE_H_

#include <vector>

#include "muehle/mini_max/pred_vars.h"
#include "muehle/mini_max/state_address_struct.h"
#include "muehle/mini_max/type_def.h"

namespace muehle {
namespace mini_max {

class GameInterface {
 public:
  /* Types */
  using uint_1d = std::vector<unsigned int>;

  /* Init */
  /* Is called before any databse calculation, any test, any GetBestChoice call
   */
  virtual void PrepareCalculation() {};

  /* Getter */
  /*Selects either the AlphaBeta or the RetroAnalysis algorithm for each layer
   */
  virtual bool ShallRetroAnalysisBeUsed(unsigned int layer_num) {
    return false;
  };
  /* Returns the possible move ids for the current player (Which can be ego and
   * opponent) */
  virtual void GetPossibilities(unsigned int thread_no,
                                std::vector<unsigned int>& possibility_ids) {};
  /* Returns the maximum number of possibilities for a move */
  virtual unsigned int GetMaxNumPossibilities() {
    return 0;
  };
  /* Total number of layers */
  virtual unsigned int GetNumberOfLayers() {
    return 0;
  };
  /* Total number of knots for a given layer */
  virtual unsigned int GetNumberOfKnotsInLayer(unsigned int layer_num) {
    return 0;
  };
  /* Maximum number of plies for the game, although this alue is not known in
   * advance, an upper boundary value must be provided for the AlphaBeta
   * algorithm */
  virtual unsigned int GetMaxNumPlies() {
    return 0;
  };

  /* Layers which can be reached by a move from each knot of the passed layer */
  virtual void GetSuccLayers(unsigned int layer_num,
                             std::vector<unsigned int>& succ_layers) {};

  /* The partner layers are calculated together at the same time */
  virtual uint_1d GetPartnerLayers(unsigned int layer_num) {
    return {};
  };
  /* Value of situation for the current player (which can be ego and opponent)
   */
  virtual void GetValueOfSituation(unsigned int thread_no, float& float_value,
                                   TwoBit& short_value) {};

  /* Returns the address of the current considered state for each thread for the
   * view of the current player (which can be ego or opponent) */
  virtual void GetLayerAndStateNumber(unsigned int thread_no,
                                      unsigned int& layer_num,
                                      unsigned int& state_number,
                                      unsigned int& sym_op) {};

  /* Gets the layer number for the state, which is active for each thread */
  virtual unsigned int GetLayerNumber(unsigned int thead_no) {
    return 0;
  };
  /* Get all the symmetric states from the current one, since all of them have
   * the same knot value*/
  virtual void GetSymStateNumWithDuplicates(
      unsigned int thead_no, std::vector<StateAddressStruct>& sym_states) {};

  /* Get al preceeding states for retro analysis */
  virtual void GetPredecessors(
      unsigned int thread_no,
      std::vector<retro_analysis::PredVars>& pred_vars) {};
  /* Do some checks if the state variables are consistent to each other */
  virtual bool IsStateIntegrityOk(unsigned int thread_no) {
    return false;
  };
  /* Apply this (inverse) symmetry operation on the current state of a certain
   * thread */
  virtual void ApplySymOp(unsigned int thread_no,
                          unsigned char symmetry_operation_number,
                          bool do_inverse_operation,
                          bool player_to_move_changed) {};

  /* Does it mean that the game is lost, when unable to move? */
  virtual bool LostIfUnableToMove(unsigned int thread_no) {
    return false;
  };

  /* Setter */
  /* Set a certain game state for a thread, even if the state is invalid, the
   * function should set the state and return false */
  virtual bool SetSituation(unsigned int thread_no, unsigned int layer_num,
                            unsigned int state_number) {
    return false;
  };

  /* Do a move based on the ids returned by GetPossibilities() */
  virtual void Move(unsigned int thread_no, unsigned int id_possibility,
                    bool& player_to_move_changed, void*& p_backup) {};
  /* Undo a move based on the ids returned by GetPossibilities() */
  virtual void Undo(unsigned int thread_no, unsigned int id_possibility,
                    bool& player_to_move_changed, void* p_backup) {};

  /* Output */
  /* For console */
  virtual void PrintField(unsigned int thread_no, TwoBit value,
                          unsigned int indent_spaces = 0) {};
  /* For console */
  virtual void PrintMoveInformation(unsigned int thead_no,
                                    unsigned int id_possibility) {};
  /* For GUI */
  virtual std::wstring GetOutputInformation(unsigned int layer_num) {
    return std::wstring(/* s: */ L"");
  }
};

} /* namespace mini_max */
} /* namespace muehle */

#endif /* MUEHLE_MINI_MAX_GAME_INTERFACE_H_ */
