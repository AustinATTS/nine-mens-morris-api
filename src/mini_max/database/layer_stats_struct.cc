#include "muehle/mini_max/database/layer_stats_struct.h"

namespace muehle {
namespace mini_max {
namespace database {

bool LayerStatsStruct::operator==(const LayerStatsStruct& other) const {
  bool is_equal = completed_and_in_file == other.completed_and_in_file &&
                 partner_layers == other.partner_layers &&
                 knots_in_layer == other.knots_in_layer &&
                 num_won_states == other.num_won_states &&
                 num_lost_states == other.num_lost_states &&
                 num_drawn_states == other.num_drawn_states &&
                 num_invalid_states == other.num_invalid_states &&
                 succ_layers == other.succ_layers && skv == other.skv &&
                 ply_info == other.ply_info;
  return is_equal;
}

long long LayerStatsStruct::GetLayerSizeInBytesForSkv() const {
  // Rounds up to the nearest multiple of 4 to ensure enough storage for packing
  // 2-bit values per knot.
  return ((knots_in_layer + 3) / 4) * sizeof(TwoBit);
}

long long LayerStatsStruct::GetLayerSizeInBytesForPlyInfo() const {
  return knots_in_layer * sizeof(PlyInfoVarType);
}

LayerStatsStruct::LayerStatsStruct() = default;

LayerStatsStruct::~LayerStatsStruct() = default;

}  // namespace database
}  // namespace mini_max
} // namespace muehle
