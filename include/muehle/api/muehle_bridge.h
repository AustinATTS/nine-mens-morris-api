#ifndef MUEHLE_API_MUEHLE_BRIDGE_H_
#define MUEHLE_API_MUEHLE_BRIDGE_H_

#include <array>
#include <string>
#include <vector>

#include "muehle/ai/min_max_ai.h"
#include "muehle/muehle.h"

namespace muehle {

class MuehleBridge {
public:
  struct Request {
    std::array<PlayerId, FieldStruct::size> board {};
    bool setting_phase{false};
    unsigned int total_num_stones_missing{0};
    unsigned int search_depth{0};
    PlayerId current_player{PlayerId::player_one};
  };

  struct ChoiceResult {
    unsigned int possibility_id{0};
    MoveInfo move;
    unsigned int short_value{0};
    unsigned int ply_info{0};
    std::array<unsigned int, mini_max::SKV_NUM_VALUES> freq_values_sub_moves{};
  };

  struct Response {
    bool success{false};
    std::string engine{"none"};
    unsigned int search_depth{0};
    PlayerId current_player{PlayerId::player_one};
    bool setting_phase{false};
    unsigned int total_num_stones_missing{0};
    bool game_has_finished{false};
    PlayerId winner{PlayerId::square_is_free};
    MoveInfo best_move;
    mini_max::StateInfo choice_info;
    std::vector<ChoiceResult> choices;
    std::string error;
  };

  MuehleBridge();

  void SetSearchDepth(unsigned int depth);
  Response Evaluate(const Request& req);

private:
  Muehle game;
  MinMaxAi ai;
  unsigned int search_depth{0};
};

}  // namespace muehle

#endif  // MUEHLE_API_MUEHLE_BRIDGE_H_
