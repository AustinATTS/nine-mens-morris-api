#ifndef MUEHLE_MINI_MAX_DATABASE_LAYER_STATS_STRUCT_H_
#define MUEHLE_MINI_MAX_DATABASE_LAYER_STATS_STRUCT_H_

#include "muehle/mini_max/type_def.h"

namespace muehle {
namespace mini_max {
namespace database {

#pragma pack(push, 1) /* Align the following struct to byte boundary. This guarantees stable byte order in release and debug mode */
/* Layer specific information */
struct LayerStatsStruct {
  bool completed_and_in_file = false; /* Trye if all states have been calculated and are stored in the file */
  unsigned char dummy[3]; /* Aligns 'completed_and_in_file' bool to 4 bytes for file compatability */
  unsigned int partner_layer = 0; /* Layer being calculated at the same time as this layer. This is a legacy variable, to be able to read the old database files. */
  StateNumberVarType knots_in_layer = 0; /* Number of knots of the corresponding layer */
  StateNumberVarType num_won_states = 0; /* Number of won states in this layer */
  StateNumberVarType num_lost_states = 0; /* Number of lost states in this layer */
  StateNumberVarType num_drawn_states = 0; /* Number of drawn states in this layer */
  StateNumberVarType num_invalid_states = 0; /* Number of invalid states in this layer */

  std::vector<unsigned int> partner_layers; /* Layer id relevant when switching current and opponent player */
  std::vector<unsigned int> succ_layers; /* Array containing the layer ids of the succeeding layers */
  std::vector<TwoBit> skv; /* array of size [(knots_in_layer + 3) / 4] containing the short know values */
  std::vector<PlyInfoVarType> ply_info; /* Array of size [knots_in_layer] containing the ply info for each knot in this layer */
  bool is_ply_info_resized = false; /* True if the ply info array has been resized. This is needed for the ply info array to be resized in the database file */
  bool is_skv_resized = false; /* True if the skv array has been resized. This is needed for the skv array to be resized in the database file */

  /* Only these bytes are saved to file */
  static const size_t num_bytes_layer_stats_header = sizeof(completed_and_in_file) + sizeof(dummy) + sizeof(partner_layer) + sizeof(knots_in_layer) + sizeof(num_won_states) + sizeof(num_lost_states) + sizeof(num_drawn_states) + sizeof(num_invalid_states); /* Bool is 1 byte, but alignment is 4 bytes. This depends on the compiler and the compiler settings */

  LayerStatsStruct();
  ~LayerStatsStruct();

  bool operator==(const LayerStatsStruct &other) const;
  long long GetLayerSizeInBytesForSkv() const;
  long long GetLayerSizeInBytesForPlyInfo() const;
};
#pragma pack(pop)

}  // namespace database
}  // namespace mini_max
}  // namespace muehle

#endif  // MUEHLE_MINI_MAX_DATABASE_LAYER_STATS_STRUCT_H_
