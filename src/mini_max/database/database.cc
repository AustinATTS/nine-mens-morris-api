#include "muehle/mini_max/database/database.h"

#include <atomic>

#include "muehle/mini_max/database/comp_file.h"
#include "muehle/mini_max/database/uncomp_file.h"
#ifdef _WIN32
#include <intrin.h>
#endif // _WIN32

namespace muehle {

/* constructor */
mini_max::database::Database::Database(GameInterface& gi, Logger& log)
    : game(&gi), log(log), array_infos(log) {}


/* destructor */
mini_max::database::Database::~Database() { CloseDatabase(); }

/* Name: setAsComplete() */
/* Set the database as completly calculated */
bool mini_max::database::Database::SetAsComplete() {
  if (!IsOpen()) {
    return log.Log(Logger::LogLevel::error, L"ERROR: No database file open!");
  }
  /* check if all layers are set as complete */
  for (unsigned int layer_number = 0; layer_number < db_stats.num_layers;
       layer_number++) {
    if (!GetNumberOfKnots(layer_number)) {
      continue;
    }
    if (!IsLayerCompleteAndInFile(layer_number)) {
      return log.Log(
          Logger::LogLevel::error,
          L"ERROR: Layer " + std::to_wstring(layer_number) +
              L" is not completely calculated and saved in the file!");
    }
       }
  log.Log(Logger::LogLevel::info, L"Database is set as complete.");
  db_stats.completed = true;
  return true;
}

/* Name: setLoadingOfFullLayerOnRead() */
/* When a reading operation is performed, the full layer is loaded into */
/* memory. */
bool mini_max::database::Database::SetLoadingOfFullLayerOnRead() {
  load_full_layer_on_read = true;
  return true;
}

/* Name: unload() */
/* The database file is kept open, the header information stays, but the */
/* data is unloaded from memory. */
void mini_max::database::Database::Unload() {
  for (unsigned int layer_number = 0; layer_number < db_stats.num_layers;
       layer_number++) {
    LayerStatsStruct& my_lss = layer_stats[layer_number];

    array_infos.RemoveArray(layer_number, ArrayInfoStruct::ArrayType::layer_stats,
                           my_lss.skv.size() * sizeof(TwoBit), 0);
    my_lss.skv.clear();
    my_lss.skv.shrink_to_fit();
    my_lss.is_skv_resized = false;

    array_infos.RemoveArray(layer_number, ArrayInfoStruct::ArrayType::ply_infos,
                           my_lss.ply_info.size() * sizeof(PlyInfoVarType), 0);
    my_lss.ply_info.clear();
    my_lss.ply_info.shrink_to_fit();
    my_lss.is_ply_info_resized = false;
       }
  log.Log(Logger::LogLevel::info, L"Database data unloaded from memory.");
}

/* Name: resizePlyInfo() */
/* */
bool mini_max::database::Database::ResizeSkv(LayerStatsStruct& my_lss,
                                            unsigned int layer_number) {
  /* lock the database mutex to prevent other threads from accessing the */
  /* database while resizing */
  std::lock_guard<std::mutex> lock(cs_database_mutex);

  /* check again if layer is already loaded, since another thread might have */
  /* loaded it in the meantime */
  if (!my_lss.is_skv_resized) {
    /* reserve memory for this layer & create array for skv with default value */
    my_lss.skv.resize((my_lss.knots_in_layer + 3) / 4, SKV_WHOLE_BYTE_IS_INVALID);

    /* if layer is in database and completed, then load layer from file into */
    /* memory, set default value otherwise */
    if (my_lss.completed_and_in_file) {
      if (!file->ReadSkv(layer_number, my_lss.skv)) {
        return log.Log(Logger::LogLevel::error,
                       L"ERROR: Reading skv of layer " +
                           std::to_wstring(layer_number) + L" from file failed!");
      }
    }

    if (!array_infos.AddArray(layer_number,
                             ArrayInfoStruct::ArrayType::layer_stats,
                             my_lss.skv.size() * sizeof(TwoBit), 0)) {
      return log.Log(Logger::LogLevel::error,
                     L"ERROR: Adding array to arrayInfos failed!");
                             }

    my_lss.is_skv_resized = true;
  }
  return true;
}

/* Name: resizePlyInfo() */
/* */
bool mini_max::database::Database::ResizePlyInfo(LayerStatsStruct& my_lss,
                                                unsigned int layer_number) {
  /* lock the database mutex to prevent other threads from accessing the */
  /* database while resizing */
  std::lock_guard<std::mutex> lock(cs_database_mutex);

  /* check again if layer is already loaded, since another thread might have */
  /* loaded it in the meantime */
  if (!my_lss.is_ply_info_resized) {
    /* reserve memory for this layer & create array for ply info with default */
    /* value */
    my_lss.ply_info.resize(my_lss.knots_in_layer, PLYINFO_VALUE_UNCALCULATED);

    /* if layer is in database and completed, then load layer from file into */
    /* memory; set default value otherwise */
    if (my_lss.completed_and_in_file) {
      if (!file->ReadPlyInfo(layer_number, my_lss.ply_info)) {
        return log.Log(Logger::LogLevel::error,
                       L"ERROR: Reading ply info of layer " +
                           std::to_wstring(layer_number) + L" from file failed!");
      }
    }

    /* statistics */
    if (!array_infos.AddArray(layer_number, ArrayInfoStruct::ArrayType::ply_infos,
                             my_lss.ply_info.size() * sizeof(PlyInfoVarType),
                             0)) {
      return log.Log(Logger::LogLevel::error,
                     L"ERROR: Adding array to arrayInfos failed!");
                             }

    my_lss.is_ply_info_resized = true;
  }
  return true;
}

/* Name: openDatabase() */
/* Open the database file and load the header information. */
bool mini_max::database::Database::OpenDatabase(std::wstring const& file_directory,
                                               bool use_comp_file_if_both_exist) {
  /* do not open the database if it is already open */
  if (file) {
    return log.Log(Logger::LogLevel::error,
                   L"ERROR: Database file already open!");
  }

  /* check if database path is valid */
  if (file_directory.size() && !std::filesystem::exists(file_directory)) {
    return log.Log(Logger::LogLevel::error, std::wstring{L"ERROR: Database path "} +
                                                file_directory +
                                                std::wstring{L" not valid!"});
  }

  /* check which database file exists */
  bool has_comp = std::filesystem::exists(file_directory + L"/database.dat");
  bool has_uncomp = std::filesystem::exists(file_directory + L"/shortKnotValue.dat") &&
                   std::filesystem::exists(file_directory + L"/plyInfo.dat");
  if (has_comp && (!has_uncomp || (has_uncomp && use_comp_file_if_both_exist))) {
    log.Log(Logger::LogLevel::info, L"Using compressed database file.");
    file = new CompFile{game, log};
  } else if (has_uncomp && (!has_comp || (has_comp && !use_comp_file_if_both_exist))) {
    log.Log(Logger::LogLevel::info, L"Using uncompressed database files.");
    file = new UncompFile{game, log};
  } else {
    log.Log(Logger::LogLevel::info,
            L"No database file found, a new one will be created.");
    file = new UncompFile{game, log};
  }

  /* open and load header */
  if (!file->OpenDatabase(file_directory)) {
    delete file;
    file = nullptr;
    return log.Log(Logger::LogLevel::error, L"ERROR: Opening database file " +
                                                file_directory + L" failed!");
  }
  if (!file->LoadHeader(db_stats, layer_stats)) {
    delete file;
    file = nullptr;
    return log.Log(Logger::LogLevel::error,
                   L"ERROR: Loading database header failed!");
  }
  log << L"Database reports " << layer_stats.size()
    << L" layers." << "\n";

  log << L"db_stats.num_layers = " << db_stats.num_layers << "\n";
  array_infos.Init(GetNumLayers());
  log.Log(Logger::LogLevel::info, L"Database opened.");

  return true;
}

/* Name: closeDatabase() */
/* Close the database file and unload all data from memory. */
bool mini_max::database::Database::CloseDatabase() {
  if (!file) {
    log.Log(Logger::LogLevel::info,
            L"Skipping closeDatabase(): No database file open!");
    return false;
  }
  delete file;
  file = nullptr;
  Unload();
  return true;
}

/* Name: saveHeader() */
/* Save the header information to the database file. */
bool mini_max::database::Database::SaveHeader() {
  if (!file) {
    return log.Log(Logger::LogLevel::error, L"ERROR: No database file open!");
  }
  return file->SaveHeader(db_stats, layer_stats);
}

/* Name: removeDatabaseFiles() */
/* Remove the database files from the file system. */
bool mini_max::database::Database::RemoveDatabaseFiles() {
  if (!file) {
    return log.Log(Logger::LogLevel::error, L"ERROR: No database file open!");
  }
  return file->RemoveFile(file->GetFileDirectory());
}

/* Name: updateLayerStats() */
/* Count the number of states in the layer - the number of won, lost, */
/* drawn and invalid states */
bool mini_max::database::Database::UpdateLayerStats(unsigned int layer_number) {
  /* checks */
  if (layer_number >= layer_stats.size()) {
    return log.Log(
        Logger::LogLevel::error,
        L"ERROR: Layer " + std::to_wstring(layer_number) + L" does not exist!");
  }

  /* locals */
  StateAddressStruct cur_state;
  unsigned int stats_value_counter[] = {0, 0, 0, 0};
  TwoBit cur_state_value;

  /* calc and show statistics */
  for (cur_state.layer_number = layer_number, cur_state.state_number = 0;
       cur_state.state_number < GetNumberOfKnots(cur_state.layer_number);
       cur_state.state_number++) {
    /* get state value */
    if (!ReadKnotValueFromDatabase(cur_state.layer_number, cur_state.state_number,
                                   cur_state_value)) {
      return log.Log(Logger::LogLevel::error,
                     L"ERROR: Reading knot value from database failed!");
                                   }
    stats_value_counter[cur_state_value]++;
       }

  /* store statistics */
  layer_stats[layer_number].num_won_states = stats_value_counter[SKV_VALUE_GAME_WON];
  layer_stats[layer_number].num_lost_states =
      stats_value_counter[SKV_VALUE_GAME_LOST];
  layer_stats[layer_number].num_drawn_states =
      stats_value_counter[SKV_VALUE_GAME_DRAWN];
  layer_stats[layer_number].num_invalid_states =
      stats_value_counter[SKV_VALUE_INVALID];

  log << L"Statistics of layer " << layer_number << L" updated." << "\n";
  return true;
}

/* Name: showLayerStats() */
/* Print the statistics of the layer to the log */
void mini_max::database::Database::ShowLayerStats(unsigned int layer_number) {
  if (layer_number >= layer_stats.size()) {
    log.Log(Logger::LogLevel::error, L"ERROR: ShowLayerStats(): Layer " +
                                         std::to_wstring(layer_number) +
                                         L" does not exist!");
    return;
  }

  log << "STATISTICS OF LAYER " << layer_number << "\n";
  log << (game->GetOutputInformation(layer_number)) << "\n";
  log << " number  states: " << GetNumberOfKnots(layer_number) << "\n";
  log << " won     states: " << GetNumWonStates(layer_number) << "\n";
  log << " lost    states: " << GetNumLostStates(layer_number) << "\n";
  log << " draw    states: " << GetNumDrawnStates(layer_number) << "\n";
  log << " invalid states: " << GetNumInvalidStates(layer_number) << "\n";
}
#pragma endregion

#pragma region getter
/* Name: isComplete() */
/* Returns true if all layers of the database are completely calculated. */
bool mini_max::database::Database::IsComplete() { return db_stats.completed; }

/* Name: isLayerCompleteAndInFile() */
/* Returns true if the layer is completely calculated and saved in the */
/* file. */
bool mini_max::database::Database::IsLayerCompleteAndInFile(
    unsigned int layer_number) {
  if (layer_number >= layer_stats.size()) {
    return log.Log(
        Logger::LogLevel::error,
        L"ERROR: Layer " + std::to_wstring(layer_number) + L" does not exist!");
  }
  return layer_stats[layer_number].completed_and_in_file;
}

/* Name: getNumberOfKnots() */
/* Returns the number of knots in the layer. */
unsigned int mini_max::database::Database::GetNumberOfKnots(
    unsigned int layer_number) {
  if (layer_number >= layer_stats.size()) return 0;
  return layer_stats[layer_number].knots_in_layer;
}

/* Name: getLayerSizeInBytes() */
/* Returns the size of the layer in bytes, which might differ from the */
/* number of knots. */
long long mini_max::database::Database::GetLayerSizeInBytes(
    unsigned int layer_num) {
  if (layer_num >= layer_stats.size()) return 0;
  return layer_stats[layer_num].GetLayerSizeInBytesForSkv() +
         layer_stats[layer_num].GetLayerSizeInBytesForPlyInfo();
}

/* Name: getNumWonStates() */
/* Returns the number of won states in the layer. */
mini_max::StateNumberVarType mini_max::database::Database::GetNumWonStates(
    unsigned int layer_num) {
  if (layer_num >= layer_stats.size()) return 0;
  return layer_stats[layer_num].num_won_states;
}

/* Name: getNumLostStates() */
/* Returns the number of lost states in the layer. */
mini_max::StateNumberVarType mini_max::database::Database::GetNumLostStates(
    unsigned int layer_num) {
  if (layer_num >= layer_stats.size()) return 0;
  return layer_stats[layer_num].num_lost_states;
}

/* Name: getNumDrawnStates() */
/* Returns the number of drawn states in the layer. */
mini_max::StateNumberVarType mini_max::database::Database::GetNumDrawnStates(
    unsigned int layer_num) {
  if (layer_num >= layer_stats.size()) return 0;
  return layer_stats[layer_num].num_drawn_states;
}

/* Name: getNumInvalidStates() */
/* Returns the number of invalid states in the layer. */
mini_max::StateNumberVarType mini_max::database::Database::GetNumInvalidStates(
    unsigned int layer_num) {
  if (layer_num >= layer_stats.size()) return 0;
  return layer_stats[layer_num].num_invalid_states;
}
#pragma endregion

/* Name: saveLayerToFile() */
/* Save the layer to the file, which is already in memory. */
/* The layer is marked as completed and saved in the file. */
/* The layer is not saved if it is empty. */
/* The header is not saved, and must be saved separately. */
bool mini_max::database::Database::SaveLayerToFile(unsigned int layer_number) {
  /* checks */
  if (!file || !IsOpen()) {
    return log.Log(Logger::LogLevel::error, L"ERROR: No database file open!");
  }
  if (layer_number >= layer_stats.size()) {
    return log.Log(
        Logger::LogLevel::error,
        L"ERROR: Layer " + std::to_wstring(layer_number) + L" does not exist!");
  }

  /* don't save layer and header when only preparing layers */
  LayerStatsStruct& my_lss = layer_stats[layer_number];

  /* save layer if there are any states */
  if (!my_lss.skv.size()) {
    return log.Log(Logger::LogLevel::error,
                   L"ERROR: Layer " + std::to_wstring(layer_number) + L" is empty!");
  }
  if (!my_lss.ply_info.size()) {
    return log.Log(
        Logger::LogLevel::error,
        L"ERROR: Ply info of layer " + std::to_wstring(layer_number) + L" is empty!");
  }

  cur_action = Activity::saving_layer_to_file;
  log << L"Saving layer " << layer_number << L" to file..." << "\n";

  /* write layer to file */
  if (!file->WriteSkv(layer_number, my_lss.skv)) {
    return log.Log(Logger::LogLevel::error, L"ERROR: Writing skv of layer " +
                                                std::to_wstring(layer_number) +
                                                L" to file failed!");
  }
  if (!file->WritePlyInfo(layer_number, my_lss.ply_info)) {
    return log.Log(Logger::LogLevel::error,
                   L"ERROR: Writing ply info of layer " +
                       std::to_wstring(layer_number) + L" to file failed!");
  }

  /* mark layer as completed */
  log << L"Layer " << layer_number << L" saved to file." << "\n";
  my_lss.completed_and_in_file = true;
  return true;
}

/* Name: loadLayerFromFile() */
/* Load the layer from the file into memory. */
/* The layer must be marked as completed and already saved in the */
/* file. */
/* The layer is not load if it not complete. */
/* The header is not loaded, and must be loaded in advance. */
bool mini_max::database::Database::LoadLayerFromFile(unsigned int layer_number) {
  /* checks */
  if (!file || !IsOpen()) {
    return log.Log(Logger::LogLevel::error, L"ERROR: No database file open!");
  }
  if (layer_number >= layer_stats.size()) {
    return log.Log(
        Logger::LogLevel::error,
        L"ERROR: Layer " + std::to_wstring(layer_number) + L" does not exist!");
  }

  LayerStatsStruct& my_lss = layer_stats[layer_number];

  /* don't load layer if not complete */
  if (!my_lss.completed_and_in_file) {
    return log.Log(Logger::LogLevel::error,
                   L"ERROR: Layer " + std::to_wstring(layer_number) +
                       L" is not completely calculated and saved in the file!");
  }

  cur_action = Activity::loading_layer_from_file;
  log << L"Loading layer " << layer_number << L" from file..." << "\n";

  /* load layer from file */
  ResizeSkv(my_lss, layer_number);
  ResizePlyInfo(my_lss, layer_number);

  log << L"Layer " << layer_number << L" loaded from file." << "\n";
  return true;
}

/* Name: readKnotValueFromDatabase() */
/* Read the knot value from the database. */
/* If the layer is in memory, the data is read from memory. */
/* If the layer is not in memory, the data is loaded from the file */
/* into memory. */
/* Apart from changes in the header information, reading is thread safe. */
bool mini_max::database::Database::ReadKnotValueFromDatabase(
    unsigned int layer_number, unsigned int state_number, TwoBit& knot_value) {
  /* checks */
  if (layer_number >= layer_stats.size() || layer_number > db_stats.num_layers) {
    knot_value = SKV_VALUE_INVALID;
    return log.Log(
        Logger::LogLevel::error,
        L"ERROR: INVALID layerNumber in readKnotValueFromDatabase()!");
  }
  if (state_number >= layer_stats[layer_number].knots_in_layer) {
    knot_value = SKV_VALUE_INVALID;
    return log.Log(
        Logger::LogLevel::error,
        L"ERROR: INVALID stateNumber in readKnotValueFromDatabase()!");
  }

  /* locals */
  TwoBit database_byte;
  TwoBit def_value = SKV_WHOLE_BYTE_IS_INVALID;
  LayerStatsStruct& my_lss = layer_stats[layer_number];

  /* valid state and layer number ? */
  if (state_number >= my_lss.knots_in_layer) {
    knot_value = SKV_VALUE_INVALID;
    return log.Log(
        Logger::LogLevel::error,
        L"ERROR: INVALID stateNumber in readKnotValueFromDatabase()!");
  }

  /* if database is complete get just single byte from file directly */
  if ((db_stats.completed || my_lss.completed_and_in_file) && !load_full_layer_on_read) {
    std::lock_guard<std::mutex> lock(cs_database_mutex);
    file->ReadSkv(layer_number, database_byte, state_number);
  } else {
    /* if layer not already loaded */
    if (!my_lss.is_skv_resized) {
      ResizeSkv(my_lss, layer_number);
    }

    /* read knot value from array */
    database_byte = my_lss.skv[state_number / 4];

    /* measure io-operations per second */
    if (MEASURE_IOPS) speedo_read_skv.MeasureIops();
  }

  /* make half byte */
  knot_value = _rotr8(database_byte, 2 * (state_number % 4)) & 3;

  return true;
}

/* Name: readPlyInfoFromDatabase() */
/* Read the ply info from the database. */
/* If the layer is in memory, the data is read from memory. */
/* If the layer is not in memory, the data is loaded from the file */
/* into memory. */
/* Apart from changes in the header information, reading is thread safe. */
bool mini_max::database::Database::ReadPlyInfoFromDatabase(
    unsigned int layer_number, unsigned int state_number, PlyInfoVarType& value) {
  /* checks */
  if (layer_number >= layer_stats.size() || layer_number > db_stats.num_layers) {
    value = PLYINFO_VALUE_INVALID;
    return log.Log(Logger::LogLevel::error,
                   L"ERROR: INVALID layerNumber in readPlyInfoFromDatabase()!");
  }
  if (state_number >= layer_stats[layer_number].knots_in_layer) {
    value = PLYINFO_VALUE_INVALID;
    return log.Log(Logger::LogLevel::error,
                   L"ERROR: INVALID stateNumber in readPlyInfoFromDatabase()!");
  }

  /* locals */
  LayerStatsStruct& my_lss = layer_stats[layer_number];

  /* valid state and layer number ? */
  if (state_number > my_lss.knots_in_layer) {
    value = PLYINFO_VALUE_INVALID;
    return log.Log(Logger::LogLevel::error,
                   L"ERROR: INVALID stateNumber in readPlyInfoFromDatabase()!");
  }

  /* if database is complete get whole byte from file */
  if ((db_stats.completed || my_lss.completed_and_in_file) && !load_full_layer_on_read) {
    std::lock_guard<std::mutex> lock(cs_database_mutex);
    file->ReadPlyInfo(layer_number, value, state_number);
  } else {
    /* is layer already in memory? */
    if (!my_lss.is_ply_info_resized) {
      ResizePlyInfo(my_lss, layer_number);
    }

    /* read ply info from array */
    value = my_lss.ply_info[state_number];

    /* measure io-operations per second */
    if (MEASURE_IOPS) speedo_read_ply.MeasureIops();
  }

  return true;
}

/* Name: writeKnotValueInDatabase() */
/* Save the knot value in the database. */
/* If the layer is in memory, the data is saved to memory. */
/* If the layer is not in memory, the data is loaded from the file */
/* into memory. */
/* Apart from changes in the header information, writing is thread safe. */
/* If the layer is already completed and in the file, the function returns */
/* false. */
bool mini_max::database::Database::WriteKnotValueInDatabase(
    unsigned int layer_number, unsigned int state_number, TwoBit knot_value) {
  /* checks */
  if (layer_number >= layer_stats.size() || layer_number > db_stats.num_layers) {
    return log.Log(
        Logger::LogLevel::error,
        L"ERROR: INVALID layerNumber in writeKnotValueInDatabase()!");
  }
  if (knot_value >= SKV_NUM_VALUES) {
    return log.Log(Logger::LogLevel::error,
                   L"ERROR: INVALID knotValue in writeKnotValueInDatabase()!");
  }
  if (state_number >= layer_stats[layer_number].knots_in_layer) {
    return log.Log(
        Logger::LogLevel::error,
        L"ERROR: INVALID stateNumber in writeKnotValueInDatabase()!");
  }

  /* locals */
  TwoBit def_value = SKV_WHOLE_BYTE_IS_INVALID;
  LayerStatsStruct& my_lss = layer_stats[layer_number];

  /* valid state and layer number ? */
  if (state_number >= my_lss.knots_in_layer) {
    return log.Log(
        Logger::LogLevel::error,
        L"ERROR: INVALID stateNumber in writeKnotValueInDatabase()!");
  }

  /* is layer already completed ? */
  if (my_lss.completed_and_in_file) {
    return log.Log(Logger::LogLevel::error,
                   L"ERROR: layer already completed and in file! function: "
                   L"writeKnotValueInDatabase()!");
  }

  /* is layer already loaded? */
  if (!my_lss.skv.size()) {
    ResizeSkv(my_lss, layer_number);
  }

  /* set value */
  int32_t* p_short_knot_value =
      ((int32_t*)&my_lss.skv[0]) + state_number / ((sizeof(int32_t) * 8) / 2);
  int32_t num_bits_to_shift =
      2 *
      (state_number % ((sizeof(int32_t) * 8) / 2));  /* little-endian byte-order */
  int32_t mask = 0x00000003 << num_bits_to_shift;
  int32_t cur_short_knot_value_long, new_short_knot_value_long;
  std::atomic_ref<int32_t> atomic_short_knot_value(*p_short_knot_value);

  do {
    cur_short_knot_value_long = atomic_short_knot_value.load();
    new_short_knot_value_long =
        (cur_short_knot_value_long & (~mask)) + (knot_value << num_bits_to_shift);
  } while (!atomic_short_knot_value.compare_exchange_weak(cur_short_knot_value_long,
                                                       new_short_knot_value_long));

  /* measure io-operations per second */
  if (MEASURE_IOPS) speedo_write_skv.MeasureIops();

  return true;
}

/* Name: writePlyInfoInDatabase() */
/* Save the ply info in the database. */
/* If the layer is in memory, the data is saved to memory. */
/* If the layer is not in memory, the data is loaded from the file */
/* into memory. */
/* Apart from changes in the header information, writing is thread safe. */
/* If the layer is already completed and in the file, the function returns */
/* false. */
bool mini_max::database::Database::WritePlyInfoInDatabase(
    unsigned int layer_number, unsigned int state_number, PlyInfoVarType value) {
  /* checks */
  if (layer_number >= layer_stats.size() || layer_number > db_stats.num_layers) {
    return log.Log(Logger::LogLevel::error,
                   L"ERROR: INVALID layerNumber in writePlyInfoInDatabase()!");
  }
  if (state_number >= layer_stats[layer_number].knots_in_layer) {
    return log.Log(Logger::LogLevel::error,
                   L"ERROR: INVALID stateNumber in writePlyInfoInDatabase()!");
  }
  if ((value > PLYINFO_EXP_VALUE && value < PLYINFO_VALUE_DRAWN) ||
      value > PLYINFO_VALUE_INVALID) {
    return log.Log(Logger::LogLevel::error,
                   L"ERROR: INVALID value in writePlyInfoInDatabase()!");
      }

  /* locals */
  LayerStatsStruct& my_lss = layer_stats[layer_number];

  /* valid state and layer number ? */
  if (state_number >= my_lss.knots_in_layer) {
    return log.Log(Logger::LogLevel::error,
                   L"ERROR: INVALID stateNumber in writePlyInfoInDatabase()!");
  }

  /* is layer already completed ? */
  if (my_lss.completed_and_in_file) {
    return log.Log(Logger::LogLevel::error,
                   L"ERROR: layer already completed and in file! function: "
                   L"writePlyInfoInDatabase()!");
  }

  /* is layer already loaded */
  if (!my_lss.ply_info.size()) {
    ResizePlyInfo(my_lss, layer_number);
  }

  /* set value */
  my_lss.ply_info[state_number] = value;

  /* measure io-operations per second */
  if (MEASURE_IOPS) speedo_write_ply.MeasureIops();

  return true;
}

}
