#ifndef MUEHLE_MINI_MAX_DATABASE_GENERIC_FILE_H_
#define MUEHLE_MINI_MAX_DATABASE_GENERIC_FILE_H_

#include "muehle/mini_max/database/database_stats_struct.h"
#include "muehle/mini_max/database/layer_stats_struct.h"

namespace muehle {
namespace mini_max {
namespace database {

const int SKV_FILE_HEADER_CODE =
    0xF4f5; /* Constant to identify the header. These are the first two bytes of
               the file */
const int PLYINFO_HEADER_CODE = 0xF3F2;

/* This is a generic glass for reading and writing the database files. It is
 * used by the classes UncompFile and CompFile. The database is stored in memory
 * and in an uncompressed file. After calculation the database is converted to a
 * compressed file. The uncompressed database is stored in two files:
 * short_knot_value.dat and ply_info.dat the compressed database is stored in
 * one file: database.dat The database files are located in the folder
 * file_directory. The database is organised into layers. Each layer contains a
 * number of knots. Each knot contains a number of states. Writing is supposed
 * for one whole layer at a time. Reading is supposed for one whole layer at a
 * time or for one state of a layer. All reading/writing operations are directly
 * accessing the database files. The database files are opened and closed by the
 * functions OpenDatabase() and CloseDatabase(), prior to and after
 * reading/writing. The database files are NOT thread safe. */
class GenericFile {
 public:
  virtual bool OpenDatabase(std::wstring const& file_directory) = 0;
  virtual void CloseDatabase() {};
  virtual bool RemoveFile(std::wstring const& file_directory = L"") {
    return false;
  };
  virtual bool IsOpen() { return false; };
  virtual bool LoadHeader(DatabaseStatsStruct& db_stats,
                          std::vector<LayerStatsStruct>& layer_stats) {
    return false;
  };
  virtual bool SaveHeader(const DatabaseStatsStruct& db_stats,
                          const std::vector<LayerStatsStruct>& layer_stats) {
    return false;
  };
  virtual bool ReadSkv(unsigned int layer_num, std::vector<TwoBit>& skv) {
    return false;
  };
  virtual bool ReadSkv(unsigned int layer_num, TwoBit& database_byte, unsigned int state_number) {
    return false;
  };
  virtual bool WriteSkv(unsigned int layer_num, const std::vector<TwoBit>& skv) {
    return false;
  };
  virtual bool ReadPlyInfo(unsigned int layer_num,
                           std::vector<PlyInfoVarType>& ply_info) {
    return false;
  };
  virtual bool ReadPlyInfo(unsigned int layer_num, PlyInfoVarType& single_ply_info, unsigned int state_number) {
    return false;
  };
  virtual bool WritePlyInfo(unsigned int layer_num, const std::vector<PlyInfoVarType>& ply_info) {
    return false;
  }
  virtual ~GenericFile() { CloseDatabase(); };
  std::wstring GetFileDirectory() { return file_directory; };

 protected:
  GenericFile(GameInterface* game, Logger& log) : game{game}, log{log} {};

  Logger& log;                   /* Logger */
  std::wstring file_directory;   /* Path of the folder where the database files
                                    are located */
  GameInterface* game = nullptr; /* Master class */
};

}  // namespace database
}  // namespace mini_max
}  // namespace muehle

#endif  // MUEHLE_MINI_MAX_DATABASE_GENERIC_FILE_H_
