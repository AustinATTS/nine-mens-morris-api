#ifndef MUEHLE_MINI_MAX_DATABASE_LAYER_STATS_STRUCT_H_
#define MUEHLE_MINI_MAX_DATABASE_LAYER_STATS_STRUCT_H_

#include "muehle/mini_max/type_def.h"

namespace muehle {
namespace mini_max {
namespace database {

/* Align the following struct to byte boundary. This guarantees stable byte
 * order in release and debug mode */
#pragma pack(push, 1)

/* Layer specific information */
struct LayerStatsStruct {
  /* True if all states have been calculated and are stored in the file */
  bool completed_and_in_file = false;

  /* Aligns 'completed_and_in_file' bool to 4 bytes for file compatability */
  unsigned char dummy[3];

  /* Layer being calculated at the same time as this layer. This is a legacy
   * variable, to be able to read the old database files. */
  unsigned int partner_layer = 0;

  /* Number of knots of the corresponding layer */
  StateNumberVarType knots_in_layer = 0;

  /* Number of won states in this layer */
  StateNumberVarType num_won_states = 0;

  /* Number of lost states in this layer */
  StateNumberVarType num_lost_states = 0;

  /* Number of drawn states in this layer */
  StateNumberVarType num_drawn_states = 0;

  /* Number of invalid states in this layer */
  StateNumberVarType num_invalid_states = 0;

  /* Layer id relevant when switching current and opponent player */
  std::vector<unsigned int> partner_layers;

  /* Array containing the layer ids of the succeeding layers */
  std::vector<unsigned int> succ_layers;

  /* Array of size [(knots_in_layer + 3) / 4] containing the short know values
   */
  std::vector<TwoBit> skv;

  /* Array of size [knots_in_layer] containing the ply info for each knot in
   * this layer */
  std::vector<PlyInfoVarType> ply_info;

  /* True if the ply info array has been resized. This is needed for the ply
   * info array to be resized in the database file */
  bool is_ply_info_resized = false;

  /* True if the skv array has been resized. This is needed for the skv array to
   * be resized in the database file */
  bool is_skv_resized = false;

  /* Only these bytes are saved to file. Bool is 1 byte, but alignment is 4
   * bytes. This depends on the compiler and the compiler settings */
  static const size_t num_bytes_layer_stats_header =
      sizeof(completed_and_in_file) + sizeof(dummy) + sizeof(partner_layer) +
      sizeof(knots_in_layer) + sizeof(num_won_states) +
      sizeof(num_lost_states) + sizeof(num_drawn_states) +
      sizeof(num_invalid_states);

  LayerStatsStruct();
  ~LayerStatsStruct();

  bool operator==(const LayerStatsStruct& other) const;
  long long GetLayerSizeInBytesForSkv() const;
  long long GetLayerSizeInBytesForPlyInfo() const;
};

#pragma pack(pop)

} /* namespace database */
} /* namespace mini_max */
} /* namespace muehle */

#endif /* MUEHLE_MINI_MAX_DATABASE_LAYER_STATS_STRUCT_H_ */
