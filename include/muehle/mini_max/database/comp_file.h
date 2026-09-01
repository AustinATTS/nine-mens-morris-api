#ifndef MUEHLE_MINI_MAX_DATABASE_COMP_FILE_H_
#define MUEHLE_MINI_MAX_DATABASE_COMP_FILE_H_
#include "database_stats_struct.h"
#include "layer_stats_struct.h"
#include "muehle/compressor/file.h"
#include "muehle/mini_max/game_interface.h"

namespace muehle {
namespace mini_max {
namespace database {

/* Compressed database file, to spare disk space during usage. The database is
 * converted to this format after calculation. */
class CompFile : public GenericFile {
 public:
  CompFile(GameInterface* game, Logger& log);
  bool OpenDatabase(std::wstring const& file_directory) override;
  void CloseDatabase() override;
  bool RemoveFile(std::wstring const& file_directory) override;
  bool LoadHeader(DatabaseStatsStruct& db_stats,
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
  bool WritePlyInfo(unsigned int layer_num,
                    const std::vector<PlyInfoVarType>& ply_info) override;

 private:
  static constexpr unsigned int block_size_in_bytes =
      10000; /* Size of one block in bytes. Each section is stored in blocks of
                this fixed size. This enables random read access. */

  compressor::PlatformCompApi comp; /* Compression algorithm */
  compressor::File file{comp}; /* Compressed database file */
  std::wstring file_name; /* Name of the database file */
  bool file_opened = false; /* True if the database file is open */
  DatabaseStatsStruct db_stats_cache; /* Own copy of the database stats, used when reading/writing the database. Updated when the header is loaded/written */
  std::vector<LayerStatsStruct> layer_stats_cache; /* Own copy of the layer stats, used when reading/writing the database. Updated when the header is loaded/written */

  bool ReadSection(const std::wstring& key, std::vector<unsigned int>& buffer);
  void UpdateFileName();
  void UpdateCache(const DatabaseStatsStruct& db_stats, const std::vector<LayerStatsStruct>& layer_stats);
};

}  // namespace database
}  // namespace mini_max
}  // namespace muehle

#endif  // MUEHLE_MINI_MAX_DATABASE_COMP_FILE_H_
