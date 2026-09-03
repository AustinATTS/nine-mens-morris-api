#ifndef MUEHLE_MINI_MAX_DATABASE_DATABASE_H_
#define MUEHLE_MINI_MAX_DATABASE_DATABASE_H_

#include <print>
#include <vector>

#include "array_info_container.h"
#include "array_info_struct.h"
#include "generic_file.h"
#include "layer_stats_struct.h"
#include "muehle/mini_max/game_interface.h"
#include "speedometer.h"

namespace muehle {
namespace mini_max {
namespace database {

class Database {
 public:
  using SuccLayerList = std::vector<unsigned int>;
  using PartnerLayerList = std::vector<unsigned int>;

  Database(GameInterface& gi, Logger& log);
  ~Database();

  /* Open and close functions */
  bool OpenDatabase(std::wstring const& file_directory,
                    bool use_comp_file_if_both_exist = true);
  bool CloseDatabase();
  bool SaveHeader();
  bool RemoveDatabaseFiles();
  void Unload();

  /* Statistics */
  void ShowLayerStats(unsigned int layer_number);
  bool UpdateLayerStats(unsigned int layer_number);

  /* Setter */
  bool SetAsComplete();
  bool SetLoadingOfFullLayerOnRead();

  /* Getter */
  bool IsOpen() {
    return file ? file->IsOpen() : false;
  };
  bool IsComplete();
  bool IsLayerCompleteAndInFile(unsigned int layer_number);
  unsigned int GetNumberOfKnots(unsigned int layer_number);
  unsigned int GetNumLayers() {
    return db_stats.num_layers;
  };
  const PartnerLayerList& GetPartnerLayers(unsigned int layer_number) {
    if (layer_number >= layer_stats.size()) {
      return partner_layer_dummy;
    }
    return layer_stats[layer_number].partner_layers;
  };
  const SuccLayerList& GetSuccLayers(unsigned int layer_number) {
    if (layer_number >= layer_stats.size()) {
      return succ_layer_dummy;
    }
    return layer_stats[layer_number].succ_layers;
  };
  long long GetMemoryUsed() {
    return array_infos.GetMemoryUsed();
  };
  StateNumberVarType GetNumWonStates(unsigned int layer_number);
  StateNumberVarType GetNumLostStates(unsigned int layer_number);
  StateNumberVarType GetNumDrawnStates(unsigned int layer_number);
  StateNumberVarType GetNumInvalidStates(unsigned int layer_number);
  long long GetLayerSizeInBytes(unsigned int layer_number);
  std::wstring GetFileDirectory() {
    if (file) {
      return file->GetFileDirectory();
    }
    return L"";
  };

  /* Read and write operations */
  bool ReadKnotValueFromDatabase(unsigned int layer_number,
                                 unsigned int state_number, TwoBit& knot_value);
  bool ReadPlyInfoFromDatabase(unsigned int layer_number,
                               unsigned int state_number,
                               PlyInfoVarType& value);
  bool WriteKnotValueInDatabase(unsigned int layer_number,
                                unsigned int state_number, TwoBit knot_value);
  bool WritePlyInfoInDatabase(unsigned int layer_number,
                              unsigned int state_number, PlyInfoVarType value);
  bool LoadLayerFromFile(unsigned int layer_number);
  bool SaveLayerToFile(unsigned int layer_number);

  /* Information about the arrays in memory */
  ArrayInfoContainer array_infos;

 private:
  /* Functions */
  bool ResizePlyInfo(LayerStatsStruct& my_lss, unsigned int layer_number);
  bool ResizeSkv(LayerStatsStruct& my_lss, unsigned int layer_number);

  /* General */

  /* Logger */
  Logger& log;

  /* Master class */
  GameInterface* game = nullptr;

  /* File handler */
  GenericFile* file = nullptr;

  /* Mutex for I/O operations */
  std::mutex cs_database_mutex;

  /* General information about the database */
  DatabaseStatsStruct db_stats;

  /* Layer specific information */
  std::vector<LayerStatsStruct> layer_stats;

  /* Dummy for empty return value */
  SuccLayerList succ_layer_dummy;

  /* Dummy for empty return value */
  PartnerLayerList partner_layer_dummy;

  /* Load full layer on read? */
  bool load_full_layer_on_read = false;

  /* Performance measurement */
  Speedometer::PrintFunctType print_iops = [&](std::wstring& name,
                                               float operations_per_sec) {};

  /* Measure database io operations per seconds of read operations */
  Speedometer speedo_read_skv{L"Read knot value ", MEASURE_TIME_FREQUENCY,
                              print_iops};

  /* Measure database io operations per second of write operations */
  Speedometer speedo_write_skv{L"Write knot value ", MEASURE_TIME_FREQUENCY,
                               print_iops};

  /* Measure database io operations per seconds of read operations */
  Speedometer speedo_read_ply{L"Read ply info ", MEASURE_TIME_FREQUENCY,
                              print_iops};

  /* Measure database io operations per second of write operations */
  Speedometer speedo_write_ply{L"Write ply info ", MEASURE_TIME_FREQUENCY,
                               print_iops};
};

} /* namespace database */
} /* namespace mini_max */
} /* namespace muehle */

#endif /* MUEHLE_MINI_MAX_DATABASE_DATABASE_H_ */