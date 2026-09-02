#ifndef MUEHLE_MINI_MAX_DATABASE_UNCOMP_FILE_H_
#define MUEHLE_MINI_MAX_DATABASE_UNCOMP_FILE_H_

#include "muehle/mini_max/database/generic_file.h"

namespace muehle {
namespace mini_max {
namespace database {

/* During calculation the database is stored in memory and in an uncompressed
 * file. After calculation the database is converted to a compressed file. */
class UncompFile : public GenericFile {
 public:
  UncompFile(GameInterface* game, Logger& log);
  ~UncompFile();

  bool OpenDatabase(std::wstring const& file_directory) override;
  void CloseDatabase() override;
  bool RemoveFile(std::wstring const& file_directory) override;
  bool LoadHeader(DatabaseStatsStruct& db_stats,
                  std::vector<LayerStatsStruct>& layer_stats) override;
  bool SaveHeader(const DatabaseStatsStruct& db_stats,
                  const std::vector<LayerStatsStruct>& layer_stats) override;
  bool IsOpen() override;
  bool ReadSkv(unsigned int layer_num, std::vector<TwoBit>& skv) override;
  bool ReadSkv(unsigned int layer_num, TwoBit& database_byte,
               unsigned int state_number) override;
  bool WriteSkv(unsigned int layer_num,
                const std::vector<TwoBit>& skv) override;
  bool ReadPlyInfo(unsigned int layer_num,
                   std::vector<PlyInfoVarType>& ply_info) override;
  bool ReadPlyInfo(unsigned int layer_num, PlyInfoVarType& single_ply_info,
                   unsigned int state_number) override;
  bool WritePlyInfo(unsigned int layer_num, const std::vector<PlyInfoVarType>& ply_info) override;

 private:
  /* Header of the short knot value file */
  struct SkvFileHeaderStruct {
    bool completed = false;      /* True if all states have been calculated */
    unsigned int num_layers = 0; /* Number of layers */
    unsigned int header_code = SKV_FILE_HEADER_CODE; /* File identifier */
    unsigned int header_and_stats_size =
        0; /* Size in bytes of this struct plus the stats */
  };

  /* Header of the ply info file */
  struct PlyInfoFileHeaderStruct {
    bool ply_info_completed = false; /* True if ply information has been
                                        calculated for all game states */
    unsigned int num_layers = 0; /* Number of layers */
    unsigned int header_code = PLYINFO_HEADER_CODE; /* File identifier */
    unsigned int header_and_ply_infos_size =
        0; /* Size in bytes of this struct plus ...*/
  };

  /* Layer specific information for the short knot value file */
  struct SkvFileLayerStruct {
    bool layer_is_completed_and_in_file =
        false; /* Layer specific information for the short knot value file */
    long long layer_offset =
        0; /* Position of this struct in the short knot value file */
    StateNumberVarType knots_in_layer =
        0; /* Number of knots of the corresponding layer */
    StateNumberVarType num_won_states =
        0; /* Number of won states in this layer */
    StateNumberVarType num_lost_states =
        0; /* Number of lost states in this layer */
    StateNumberVarType num_drawn_states =
        0; /* Number of drawn states in this later */
    StateNumberVarType num_invalid_states =
        0;                          /* Number of invalid states in this layer */
    unsigned int size_in_bytes = 0; /* (knots_in_layer + 3) / 4 */
    std::vector<unsigned int> succ_layers; /* Array containing the layer ids of
                                              the succeding layers */
    std::vector<unsigned int> partner_layers; /* layers being calculated at the
                                                 same time as this layer */

    unsigned int GetSizeInBytes() const;
    bool SaveToFile(HANDLE h_file) const;
    bool LoadFromFile(HANDLE h_file);
    bool SaveVectorToFile(HANDLE h_file,
                          const std::vector<unsigned int>& buffer) const;
    bool LoadVectorFromFile(HANDLE h_file, std::vector<unsigned int>& buffer);
  };

  /* Layer specific information for the ply info file */
  struct PlyInfoFileLayerStruct {
    bool ply_info_is_completed_and_in_file =
        false; /* True, after user called WritePlyInfo() storing all ply info in
                  the file */
    long long layer_offset =
        0; /* position of this struct in the ply info file */
    unsigned int size_in_bytes =
        0; /* Size of this struct plus the array ply_info[] */
    StateNumberVarType knots_in_layer =
        0; /* Number of knots of the corresponding layer */
  };

  HANDLE h_file_short_knot_values =
      INVALID_HANDLE_VALUE; /* Handle of the file for the short knot value */
  HANDLE h_file_ply_info =
      INVALID_HANDLE_VALUE;        /* Handle of the file for the ply info */
  SkvFileHeaderStruct skvf_header; /* Short knot value file header */
  PlyInfoFileHeaderStruct ply_info_header; /* Header of the ply info file */
  std::vector<SkvFileLayerStruct>
      my_layer_stats; /* Array of size [num_layers] containing general layer
                         information and the skv */
  std::vector<PlyInfoFileLayerStruct>
      ply_infos; /* Array of size [num_layers] containing ply information */

  bool CreateAndWriteEmptySkvHeader();
  bool CreateAndWriteEmptyPlyHeader();
  bool OpenSkvFile(DatabaseStatsStruct& db_stats,
                   std::vector<LayerStatsStruct>& layer_stats);
  bool OpenPlyInfoFile(std::vector<LayerStatsStruct>& layer_stats);
  bool SaveSkvHeader(const SkvFileHeaderStruct& db_h,
                     const std::vector<SkvFileLayerStruct>& l_stats);
  bool SavePlyHeader(const PlyInfoFileHeaderStruct& pi_h,
                     const std::vector<PlyInfoFileLayerStruct>& p_info);
  bool LoadBytesFromFile(HANDLE h_file, long long offset,
                         unsigned int num_bytes, void* p_bytes);
  bool SaveBytesToFile(HANDLE h_file, long long offset, unsigned int num_bytes,
                       const void* p_bytes);
  void UnloadDatabase();
};

}  // namespace database
}  // namespace mini_max
}  // namespace muehle

#endif  // MUEHLE_MINI_MAX_DATABASE_UNCOMP_FILE_H_
