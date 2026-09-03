#include "muehle/mini_max/database/uncomp_file.h"
#include "muehle/mini_max/game_interface.h"

namespace muehle {

// Desc: Constructor
//-----------------------------------------------------------------------------
mini_max::database::UncompFile::UncompFile(GameInterface* game, Logger& log)
    : GenericFile{game, log} {}

// Desc: Destructor
//-----------------------------------------------------------------------------
mini_max::database::UncompFile::~UncompFile() { CloseDatabase(); }

// Desc: Opens the database allowing read and write operations.
// 		 Fails if the database is already open.
//-----------------------------------------------------------------------------
bool mini_max::database::UncompFile::OpenDatabase(
    std::wstring const& file_directory) {
  // Fails if the database is already open.
  if (IsOpen())
    return log.Log(Logger::LogLevel::error, L"Database is already open.");

  // remember the directory
  this->file_directory = file_directory;

  // get file name
  std::wstringstream ss_database_file;
  ss_database_file << file_directory << (file_directory.size() ? "/" : "")
                   << "shortKnotValue.dat";
  log << "Open short knot value file: " << file_directory
      << (file_directory.size() ? "/" : "") << "shortKnotValue.dat" << "\n";

  // Open Database-File (FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH |
  // FILE_FLAG_RANDOM_ACCESS)
  h_file_short_knot_values =
      CreateFile(ss_database_file.str().c_str(), GENERIC_READ | GENERIC_WRITE,
                 FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
                 FILE_ATTRIBUTE_NORMAL, NULL);

  // opened file succesfully?
  if (h_file_short_knot_values == INVALID_HANDLE_VALUE)
    return log.Log(Logger::LogLevel::error,
                   L"Failed to open short knot value file.");

  // get file name
  std::wstringstream ss_file;
  ss_file << file_directory << (file_directory.size() ? "/" : "")
          << "plyInfo.dat";
  log << "Open ply info file: " << ss_file.str().c_str() << "\n";

  // Open Database-File (FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH |
  // FILE_FLAG_RANDOM_ACCESS)
  h_file_ply_info =
    CreateFile(ss_file.str().c_str(), GENERIC_READ | GENERIC_WRITE,
               FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
               FILE_ATTRIBUTE_NORMAL, NULL);

  // opened file succesfully?
  if (h_file_ply_info == INVALID_HANDLE_VALUE) {
    CloseHandle(h_file_short_knot_values);
    return log.Log(Logger::LogLevel::error, L"Failed to open ply info file.");
  }

  return true;
}

// Desc: Removes the database files from the disk.
//		 Close the database, if necessary, before deletion.
//-----------------------------------------------------------------------------
bool mini_max::database::UncompFile::RemoveFile(
    std::wstring const& file_directory) {
  std::wstringstream ss_database_file;

  if (IsOpen()) CloseDatabase();

  log << "Remove database files in folder: " << file_directory << "\n";

  ss_database_file.str(L"");
  ss_database_file << file_directory << (file_directory.size() ? "/" : "")
                   << "shortKnotValue.dat";
  if (GetFileAttributes(ss_database_file.str().c_str()) !=
      INVALID_FILE_ATTRIBUTES) {
    _wremove(ss_database_file.str().c_str());
  }

  // check if files are deleted
  if (GetFileAttributes(ss_database_file.str().c_str()) !=
      INVALID_FILE_ATTRIBUTES) {
    return log.Log(Logger::LogLevel::error,
                   L"Failed to delete database files.");
  }
  log << "Database files deleted successfully.\n";

  ss_database_file.str(L"");
  ss_database_file << file_directory << (file_directory.size() ? "/" : "")
                   << "plyInfo.dat";
  if (GetFileAttributes(ss_database_file.str().c_str()) !=
      INVALID_FILE_ATTRIBUTES) {
    _wremove(ss_database_file.str().c_str());
  }

  // check if files are deleted
  if (GetFileAttributes(ss_database_file.str().c_str()) !=
      INVALID_FILE_ATTRIBUTES) {
    return log.Log(Logger::LogLevel::error,
                   L"Failed to delete database files.");
  }
  log << "Database files deleted successfully.\n";

  return true;
}

// Desc: Loads the header and stats from the database files into memory.
//		 New files are created if they do not exist.
//		 The header is checked for consistency with the loaded files.
//       db_stats and layer_stats will be completely overwritten.
//		 Returns false if the header is not loaded.
// 		 db_stats: 	 general information about the database
// 		 layer_stats: layer specific information
//-----------------------------------------------------------------------------
bool mini_max::database::UncompFile::LoadHeader(
    DatabaseStatsStruct& db_stats, std::vector<LayerStatsStruct>& layer_stats) {
  if (!IsOpen())
    return log.Log(Logger::LogLevel::error,
                   L"Cannot load header, since database is not open.");
  if (!OpenSkvFile(db_stats, layer_stats)) return false;
  if (!OpenPlyInfoFile(layer_stats)) return false;
  log << "Database header loaded.\n";
  return true;
}

// Desc: Saves the header and stats to the database files.
//		 They are NOT checked for consistency with the written skv and
// plyInfo.
//-----------------------------------------------------------------------------
bool mini_max::database::UncompFile::SaveHeader(
    const DatabaseStatsStruct& db_stats,
    const std::vector<LayerStatsStruct>& layer_stats) {
  if (!IsOpen())
    return log.Log(Logger::LogLevel::error,
                   L"Cannot save header, since database is not open.");
  log << "Save database header.\n";

  skvf_header.completed = db_stats.completed;
  ply_info_header.ply_info_completed = db_stats.completed;
  skvf_header.num_layers = db_stats.num_layers;

  // copy infos from generic layer stats to myLayerStats and plyInfos
  my_layer_stats.resize(skvf_header.num_layers);
  for (unsigned int i = 0; i < skvf_header.num_layers; i++) {
    my_layer_stats[i].layer_is_completed_and_in_file =
        layer_stats[i].completed_and_in_file;
    my_layer_stats[i].num_won_states = layer_stats[i].num_won_states;
    my_layer_stats[i].num_lost_states = layer_stats[i].num_lost_states;
    my_layer_stats[i].num_drawn_states = layer_stats[i].num_drawn_states;
    my_layer_stats[i].num_invalid_states = layer_stats[i].num_invalid_states;
    ply_infos[i].ply_info_is_completed_and_in_file =
        layer_stats[i].completed_and_in_file;
  }

  if (!SaveSkvHeader(skvf_header, my_layer_stats)) return false;
  if (!SavePlyHeader(ply_info_header, ply_infos)) return false;
  return true;
}

// Desc: Returns true if the handles to both database files are not NULL.
//-----------------------------------------------------------------------------
bool mini_max::database::UncompFile::IsOpen() {
  return h_file_short_knot_values != INVALID_HANDLE_VALUE &&
         h_file_ply_info != INVALID_HANDLE_VALUE;
}

// Desc: Read all short knot values from the database for a given layer.
//		 The passed std::vector must have the correct size
// myLayerStats[layerNum].sizeInBytes, which can be 0.
//-----------------------------------------------------------------------------
bool mini_max::database::UncompFile::ReadSkv(unsigned int layer_num,
                                             std::vector<TwoBit>& skv) {
  if (layer_num >= my_layer_stats.size())
    return log.Log(Logger::LogLevel::error,
                   L"readSkv() failed. Layer number out of range.");
  if (skv.size() != my_layer_stats[layer_num].size_in_bytes)
    return log.Log(
        Logger::LogLevel::error,
        L"readSkv() failed. Size of passed std::vector does not match "
        L"size of layer.");
  if (h_file_short_knot_values == INVALID_HANDLE_VALUE)
    return log.Log(Logger::LogLevel::error,
                   L"readSkv() failed. Database file not open.");
  if (skvf_header.header_and_stats_size == 0)
    return log.Log(Logger::LogLevel::error,
                   L"readSkv() failed. Header not loaded.");
  if (my_layer_stats[layer_num].size_in_bytes == 0)
    return log.Log(Logger::LogLevel::error,
                   L"readSkv() failed. Layer has no knots.");
  if (!my_layer_stats[layer_num].layer_is_completed_and_in_file)
    return log.Log(Logger::LogLevel::error,
                   L"readSkv() failed. Layer is not in file.");
  return LoadBytesFromFile(h_file_short_knot_values,
                           skvf_header.header_and_stats_size +
                               my_layer_stats[layer_num].layer_offset,
                           my_layer_stats[layer_num].size_in_bytes, &skv[0]);
}

// Desc: Read a short knot value from the database for a given layer and state
// number.
//-----------------------------------------------------------------------------
bool mini_max::database::UncompFile::ReadSkv(unsigned int layer_num,
                                             TwoBit& database_byte,
                                             unsigned int state_number) {
  if (layer_num >= my_layer_stats.size())
    return log.Log(Logger::LogLevel::error,
                   L"readSkv() failed. Layer number out of range.");
  if (h_file_short_knot_values == INVALID_HANDLE_VALUE)
    return log.Log(Logger::LogLevel::error,
                   L"readSkv() failed. Database file not open.");
  if (skvf_header.header_and_stats_size == 0)
    return log.Log(Logger::LogLevel::error,
                   L"readSkv() failed. Header not loaded.");
  if (state_number >= my_layer_stats[layer_num].knots_in_layer)
    return log.Log(Logger::LogLevel::error,
                   L"readSkv() failed. State number out of range.");
  if (my_layer_stats[layer_num].size_in_bytes == 0)
    return log.Log(Logger::LogLevel::error,
                   L"readSkv() failed. Layer has no knots.");
  if (my_layer_stats[layer_num].size_in_bytes <= state_number / 4)
    return log.Log(Logger::LogLevel::error,
                   L"readSkv() failed. State number out of range.");
  if (!my_layer_stats[layer_num].layer_is_completed_and_in_file)
    return log.Log(Logger::LogLevel::error,
                   L"readSkv() failed. Layer is not in file.");
  return LoadBytesFromFile(h_file_short_knot_values,
                           skvf_header.header_and_stats_size +
                               my_layer_stats[layer_num].layer_offset +
                               state_number / 4,
                           sizeof(TwoBit), &database_byte);
}

// Desc: Write all short knot values to the database for a given layer.
//		 The passed std::vector must have the correct size
// myLayerStats[layerNum].sizeInBytes, which can be 0.
//-----------------------------------------------------------------------------
bool mini_max::database::UncompFile::WriteSkv(unsigned int layer_num,
                                              const std::vector<TwoBit>& skv) {
  if (layer_num >= my_layer_stats.size())
    return log.Log(Logger::LogLevel::error,
                   L"writeSkv() failed. Layer number out of range.");
  if (skv.size() != my_layer_stats[layer_num].size_in_bytes)
    return log.Log(
        Logger::LogLevel::error,
        L"writeSkv() failed. Size of passed std::vector does not match "
        L"size of layer.");
  if (h_file_short_knot_values == INVALID_HANDLE_VALUE)
    return log.Log(Logger::LogLevel::error,
                   L"writeSkv() failed. Database file not open.");
  if (skvf_header.header_and_stats_size == 0)
    return log.Log(Logger::LogLevel::error,
                   L"writeSkv() failed. Header not loaded.");
  if (my_layer_stats[layer_num].size_in_bytes == 0)
    return log.Log(Logger::LogLevel::error,
                   L"writeSkv() failed. Layer has no knots.");
  my_layer_stats[layer_num].layer_is_completed_and_in_file = true;
  return SaveBytesToFile(h_file_short_knot_values,
                         skvf_header.header_and_stats_size +
                             my_layer_stats[layer_num].layer_offset,
                         my_layer_stats[layer_num].size_in_bytes, &skv[0]);
}

// Desc: Read all ply information from the database for a given layer.
// 		 The passed std::vector must have the correct size
// plyInfos[layerNum].sizeInBytes, which can be 0.
//-----------------------------------------------------------------------------
bool mini_max::database::UncompFile::ReadPlyInfo(
    unsigned int layer_num, std::vector<PlyInfoVarType>& ply_info) {
  if (layer_num >= ply_infos.size())
    return log.Log(Logger::LogLevel::error,
                   L"readPlyInfo() failed. Layer number out of range.");
  if (ply_info.size() != ply_infos[layer_num].knots_in_layer)
    return log.Log(Logger::LogLevel::error,
                   L"readPlyInfo() failed. Size of passed std::vector does not "
                   L"match size of layer.");
  if (h_file_ply_info == INVALID_HANDLE_VALUE)
    return log.Log(Logger::LogLevel::error,
                   L"readPlyInfo() failed. Database file not open.");
  if (ply_info_header.header_and_ply_infos_size == 0)
    return log.Log(Logger::LogLevel::error,
                   L"readPlyInfo() failed. Header not loaded.");
  if (ply_infos[layer_num].size_in_bytes == 0)
    return log.Log(Logger::LogLevel::error,
                   L"readPlyInfo() failed. Layer has no knots.");
  if (!ply_infos[layer_num].ply_info_is_completed_and_in_file)
    return log.Log(Logger::LogLevel::error,
                   L"readPlyInfo() failed. Layer is not in file.");
  return LoadBytesFromFile(h_file_ply_info,
                           ply_info_header.header_and_ply_infos_size +
                               ply_infos[layer_num].layer_offset,
                           ply_infos[layer_num].size_in_bytes, &ply_info[0]);
}

// Desc: Read the ply information from the database for a given layer and state
// number.
//-----------------------------------------------------------------------------
bool mini_max::database::UncompFile::ReadPlyInfo(
    unsigned int layer_num, PlyInfoVarType& single_ply_info,
    unsigned int state_number) {
  if (layer_num >= ply_infos.size())
    return log.Log(Logger::LogLevel::error,
                   L"readPlyInfo() failed. Layer number out of range.");
  if (h_file_ply_info == INVALID_HANDLE_VALUE)
    return log.Log(Logger::LogLevel::error,
                   L"readPlyInfo() failed. Database file not open.");
  if (ply_info_header.header_and_ply_infos_size == 0)
    return log.Log(Logger::LogLevel::error,
                   L"readPlyInfo() failed. Header not loaded.");
  if (state_number >= ply_infos[layer_num].knots_in_layer)
    return log.Log(Logger::LogLevel::error,
                   L"readPlyInfo() failed. State number out of range.");
  if (ply_infos[layer_num].size_in_bytes == 0)
    return log.Log(Logger::LogLevel::error,
                   L"readPlyInfo() failed. Layer has no knots.");
  if (!ply_infos[layer_num].ply_info_is_completed_and_in_file)
    return log.Log(Logger::LogLevel::error,
                   L"readPlyInfo() failed. Layer is not in file.");
  return LoadBytesFromFile(h_file_ply_info,
                           ply_info_header.header_and_ply_infos_size +
                               ply_infos[layer_num].layer_offset +
                               sizeof(PlyInfoVarType) * state_number,
                           sizeof(PlyInfoVarType), &single_ply_info);
}

// Desc: Write all ply information to the database for a given layer.
//-----------------------------------------------------------------------------
bool mini_max::database::UncompFile::WritePlyInfo(
    unsigned int layer_num, const std::vector<PlyInfoVarType>& ply_info) {
  if (layer_num >= ply_infos.size())
    return log.Log(Logger::LogLevel::error,
                   L"writePlyInfo() failed. Layer number out of range.");
  if (ply_info.size() != ply_infos[layer_num].knots_in_layer)
    return log.Log(
        Logger::LogLevel::error,
        L"writePlyInfo() failed. Size of passed std::vector does not "
        L"match size of layer.");
  if (h_file_ply_info == INVALID_HANDLE_VALUE)
    return log.Log(Logger::LogLevel::error,
                   L"writePlyInfo() failed. Database file not open.");
  if (ply_info_header.header_and_ply_infos_size == 0)
    return log.Log(Logger::LogLevel::error,
                   L"writePlyInfo() failed. Header not loaded.");
  if (ply_infos[layer_num].size_in_bytes == 0)
    return log.Log(Logger::LogLevel::error,
                   L"writePlyInfo() failed. Layer has no knots.");
  ply_infos[layer_num].ply_info_is_completed_and_in_file = true;
  return SaveBytesToFile(h_file_ply_info,
                         ply_info_header.header_and_ply_infos_size +
                             ply_infos[layer_num].layer_offset,
                         ply_infos[layer_num].size_in_bytes, &ply_info[0]);
}

// Desc: Close the database files.
// 		 The data in the memory is not affected.
//-----------------------------------------------------------------------------
void mini_max::database::UncompFile::CloseDatabase() {
  log << "Close database.\n";

  // close database
  if (h_file_short_knot_values != INVALID_HANDLE_VALUE) {
    CloseHandle(h_file_short_knot_values);
    h_file_short_knot_values = INVALID_HANDLE_VALUE;
  }

  // close ply information file
  if (h_file_ply_info != INVALID_HANDLE_VALUE) {
    CloseHandle(h_file_ply_info);
    h_file_ply_info = INVALID_HANDLE_VALUE;
  }
  my_layer_stats.clear();
  ply_infos.clear();
  ply_info_header.num_layers = 0;
  ply_info_header.header_and_ply_infos_size = 0;
  ply_info_header.ply_info_completed = false;
  skvf_header.completed = false;
  skvf_header.num_layers = 0;
  skvf_header.header_and_stats_size = 0;
}

// Desc: Write a number of bytes to a file at a given offset.
//		 If operation fails, the function waits for 1 second and tries
// again.
//-----------------------------------------------------------------------------
bool mini_max::database::UncompFile::SaveBytesToFile(HANDLE h_file,
                                                     long long offset,
                                                     unsigned int num_bytes,
                                                     const void* p_bytes) {
  if (h_file == NULL)
    return log.Log(Logger::LogLevel::error,
                   L"ERROR: saveBytesToFile() failed. Database file not open.");

  DWORD dw_bytes_written;
  LARGE_INTEGER li_distance_to_move;
  unsigned int resting_bytes = num_bytes;
  const void* my_pointer = p_bytes;

  li_distance_to_move.QuadPart = offset;

  while (!SetFilePointerEx(h_file, li_distance_to_move, NULL, FILE_BEGIN)) {
    log.Log(Logger::LogLevel::error,
            std::wstring{L"ERROR: SetFilePointerEx failed!"});
    Sleep(1000);
  }

  while (resting_bytes > 0) {
    if (WriteFile(h_file, my_pointer, resting_bytes, &dw_bytes_written, NULL) ==
        TRUE) {
      resting_bytes -= dw_bytes_written;
      my_pointer = (void*)(((unsigned char*)my_pointer) + dw_bytes_written);
      if (resting_bytes > 0) {
        log << L"Still " << resting_bytes << L" to write!\n";
      }
    } else {
      log.Log(Logger::LogLevel::error,
              std::wstring{L"ERROR: WriteFile Failed!"});
      Sleep(1000);
    }
  }
  return true;
}

// Desc: Read a number of bytes from a file at a given offset.
//		 If operation fails, the function waits for 1 second and tries
// again.
//-----------------------------------------------------------------------------
bool mini_max::database::UncompFile::LoadBytesFromFile(HANDLE h_file,
                                                       long long offset,
                                                       unsigned int num_bytes,
                                                       void* p_bytes) {
  if (h_file == NULL)
    return log.Log(
        Logger::LogLevel::error,
        L"ERROR: loadBytesFromFile() failed. Database file not open.");

  DWORD dw_bytes_read;
  LARGE_INTEGER li_distance_to_move;
  unsigned int resting_bytes = num_bytes;
  void* my_pointer = p_bytes;

  li_distance_to_move.QuadPart = offset;

  while (!SetFilePointerEx(h_file, li_distance_to_move, NULL, FILE_BEGIN)) {
    log.Log(Logger::LogLevel::error,
            std::wstring{L"ERROR: SetFilePointerEx failed!"});
    Sleep(1000);
  }

  while (resting_bytes > 0) {
    if (ReadFile(h_file, p_bytes, resting_bytes, &dw_bytes_read, NULL) ==
        TRUE) {
      resting_bytes -= dw_bytes_read;
      my_pointer = (void*)(((unsigned char*)my_pointer) + dw_bytes_read);
      if (resting_bytes > 0) {
        log << L"Still " << resting_bytes << L" bytes to read!\n";
      }
    } else {
      log.Log(Logger::LogLevel::error,
              std::wstring{L"ERROR: ReadFile Failed!"});
      Sleep(1000);
    }
  }
  return true;
}

// Desc: Store the header and stats in the short knot value file.
//-----------------------------------------------------------------------------
bool mini_max::database::UncompFile::SaveSkvHeader(
    const SkvFileHeaderStruct& db_h,
    const std::vector<SkvFileLayerStruct>& l_stats) {
  if (h_file_short_knot_values == INVALID_HANDLE_VALUE)
    return log.Log(Logger::LogLevel::error,
                   L"saveSkvHeader() failed. Database file not open.");
  if (l_stats.size() != db_h.num_layers)
    return log.Log(Logger::LogLevel::error,
                   L"saveSkvHeader() failed. Number of layers does not match.");
  if (!SaveBytesToFile(h_file_short_knot_values, 0, sizeof(SkvFileHeaderStruct),
                       &db_h))
    return false;
  for (unsigned int i = 0; i < db_h.num_layers; i++) {
    if (!l_stats[i].SaveToFile(h_file_short_knot_values)) return false;
  }
  return true;
}

// Desc: Store the header and stats in the ply info file.
//-----------------------------------------------------------------------------
bool mini_max::database::UncompFile::SavePlyHeader(
    const PlyInfoFileHeaderStruct& pi_h,
    const std::vector<PlyInfoFileLayerStruct>& p_info) {
  if (h_file_ply_info == INVALID_HANDLE_VALUE)
    return log.Log(Logger::LogLevel::error,
                   L"savePlyHeader() failed. Database file not open.");
  if (p_info.size() != pi_h.num_layers)
    return log.Log(Logger::LogLevel::error,
                   L"savePlyHeader() failed. Number of layers does not match.");
  if (!SaveBytesToFile(h_file_ply_info, 0, sizeof(PlyInfoFileHeaderStruct),
                       &pi_h))
    return false;
  if (!SaveBytesToFile(h_file_ply_info, sizeof(PlyInfoFileHeaderStruct),
                       sizeof(PlyInfoFileLayerStruct) * pi_h.num_layers,
                       &p_info[0]))
    return false;
  return true;
}

// Desc: Create an empty short knot value file with the header only.
//-----------------------------------------------------------------------------
bool mini_max::database::UncompFile::CreateAndWriteEmptySkvHeader() {
  // create default header
  skvf_header.completed = false;
  skvf_header.num_layers = game->GetNumberOfLayers();
  skvf_header.header_code = SKV_FILE_HEADER_CODE;
  skvf_header.header_and_stats_size = sizeof(SkvFileHeaderStruct);
  my_layer_stats.resize(skvf_header.num_layers);
  my_layer_stats[0].layer_offset = 0;

  // locals
  for (unsigned int i = 0; i < skvf_header.num_layers; i++) {
    game->GetSuccLayers(i, my_layer_stats[i].succ_layers);
    my_layer_stats[i].partner_layers = game->GetPartnerLayers(i);
    my_layer_stats[i].knots_in_layer = game->GetNumberOfKnotsInLayer(i);
    my_layer_stats[i].size_in_bytes =
        (my_layer_stats[i].knots_in_layer + 3) / 4;
    my_layer_stats[i].layer_is_completed_and_in_file = false;
    my_layer_stats[i].num_won_states = 0;
    my_layer_stats[i].num_lost_states = 0;
    my_layer_stats[i].num_drawn_states = 0;
    my_layer_stats[i].num_invalid_states = 0;
  }

  // calculate layer offsets, based on the size of the previous layer
  for (unsigned int i = 1; i < skvf_header.num_layers; i++) {
    my_layer_stats[i].layer_offset = my_layer_stats[i - 1].layer_offset +
                                     my_layer_stats[i - 1].size_in_bytes;
  }

  // add the size of all layers to the header size
  for (auto& layer : my_layer_stats) {
    skvf_header.header_and_stats_size += layer.GetSizeInBytes();
  }

  // write header
  return SaveSkvHeader(skvf_header, my_layer_stats);
}

// Desc: Open the short knot value file and read the header and stats into the
// memory.
//	 	 If the file is invalid or does not exist, a new file with an
// empty header is created.
//-----------------------------------------------------------------------------
bool mini_max::database::UncompFile::OpenSkvFile(
    DatabaseStatsStruct& db_stats, std::vector<LayerStatsStruct>& layer_stats) {
  // locals
  size_t file_size;
  unsigned int i;

  // is file already open?
  if (h_file_short_knot_values == INVALID_HANDLE_VALUE)
    return log.Log(Logger::LogLevel::error, L"Database file not open.");

  // Get the file size
  file_size = GetFileSize(h_file_short_knot_values, NULL);

  // invalid file size ?
  if (file_size < sizeof(SkvFileHeaderStruct)) {
    // create empty header
    log << "Create empty short knot value file.\n";
    if (!CreateAndWriteEmptySkvHeader()) {
      return false;
    }

    // read layer stats
  } else {
    // database complete ?
    if (!LoadBytesFromFile(h_file_short_knot_values, 0,
                           sizeof(SkvFileHeaderStruct), &skvf_header)) {
      return false;
    }

    // invalid file ?
    if (skvf_header.header_code != SKV_FILE_HEADER_CODE)
      return log.Log(Logger::LogLevel::error,
                     L"Invalid short knot value file header.");

    // read layer stats
    my_layer_stats.resize(skvf_header.num_layers);
    for (auto& layer : my_layer_stats) {
      if (!layer.LoadFromFile(h_file_short_knot_values)) {
        return false;
      }
    }
  }

  // Translation to layer_stats and db_stats
  db_stats.completed = skvf_header.completed;
  db_stats.num_layers = skvf_header.num_layers;
  layer_stats.resize(skvf_header.num_layers);
  for (i = 0; i < skvf_header.num_layers; i++) {
    layer_stats[i].completed_and_in_file =
        my_layer_stats[i].layer_is_completed_and_in_file;
    layer_stats[i].knots_in_layer = my_layer_stats[i].knots_in_layer;
    layer_stats[i].num_won_states = my_layer_stats[i].num_won_states;
    layer_stats[i].num_lost_states = my_layer_stats[i].num_lost_states;
    layer_stats[i].num_drawn_states = my_layer_stats[i].num_drawn_states;
    layer_stats[i].num_invalid_states = my_layer_stats[i].num_invalid_states;
    layer_stats[i].partner_layer = my_layer_stats[i].partner_layers[0];
    layer_stats[i].skv.clear();
    layer_stats[i].ply_info.clear();
    layer_stats[i].succ_layers = my_layer_stats[i].succ_layers;
    layer_stats[i].partner_layers = my_layer_stats[i].partner_layers;
  }
  return true;
}

// Desc: Create an empty ply info file with the header only.
//-----------------------------------------------------------------------------
bool mini_max::database::UncompFile::CreateAndWriteEmptyPlyHeader() {
  log << "Create empty ply info file.\n";

  // create default header
  ply_info_header.ply_info_completed = false;
  ply_info_header.num_layers = game->GetNumberOfLayers();
  ply_info_header.header_code = PLYINFO_HEADER_CODE;
  ply_info_header.header_and_ply_infos_size =
      sizeof(PlyInfoFileLayerStruct) * ply_info_header.num_layers +
      sizeof(ply_info_header);
  ply_infos.resize(ply_info_header.num_layers);
  ply_infos[0].layer_offset = 0;

  for (unsigned int i = 0; i < ply_info_header.num_layers; i++) {
    ply_infos[i].knots_in_layer = game->GetNumberOfKnotsInLayer(i);
    ply_infos[i].ply_info_is_completed_and_in_file = false;
    ply_infos[i].size_in_bytes =
        ply_infos[i].knots_in_layer * sizeof(PlyInfoVarType);
  }

  for (unsigned int i = 1; i < ply_info_header.num_layers; i++) {
    ply_infos[i].layer_offset =
        ply_infos[i - 1].layer_offset + ply_infos[i - 1].size_in_bytes;
  }

  // write header
  return SavePlyHeader(ply_info_header, ply_infos);
}

// Desc: Open the ply info file and read the header and stats into the memory.
//		 If the file is invalid or does not exist, a new file with an
// empty header is created.
//-----------------------------------------------------------------------------
bool mini_max::database::UncompFile::OpenPlyInfoFile(
    std::vector<LayerStatsStruct>& layer_stats) {
  // locals
  DWORD file_size;
  unsigned int i;

  // is file already open?
  if (h_file_ply_info == INVALID_HANDLE_VALUE)
    return log.Log(Logger::LogLevel::error, L"Database file not open.");

  // Get the file size
  file_size = GetFileSize(h_file_ply_info, NULL);

  // invalid file ?
  if (file_size < sizeof(ply_info_header) + sizeof(PlyInfoFileLayerStruct) *
                                                ply_info_header.num_layers) {
    // create empty header
    if (!CreateAndWriteEmptyPlyHeader()) {
      return false;
    }

    // read layer stats
  } else {
    // database complete ?
    if (!LoadBytesFromFile(h_file_ply_info, 0, sizeof(ply_info_header),
                           &ply_info_header)) {
      return false;
    }

    // invalid file ?
    if (ply_info_header.header_code != PLYINFO_HEADER_CODE)
      return log.Log(Logger::LogLevel::error, L"Invalid ply info file header.");

    // read layer stats
    ply_infos.resize(ply_info_header.num_layers);
    if (!LoadBytesFromFile(
            h_file_ply_info, sizeof(PlyInfoFileHeaderStruct),
            sizeof(PlyInfoFileLayerStruct) * ply_info_header.num_layers,
            ply_infos.data())) {
      return false;
    }
  }
  return true;
}

unsigned int
mini_max::database::UncompFile::SkvFileLayerStruct::GetSizeInBytes() const {
  return sizeof(layer_is_completed_and_in_file) + sizeof(layer_offset) +
         sizeof(knots_in_layer) + sizeof(num_won_states) +
         sizeof(num_lost_states) + sizeof(num_drawn_states) +
         sizeof(num_invalid_states) + sizeof(size_in_bytes) +
         sizeof(unsigned int) * succ_layers.size() + sizeof(unsigned int) +
         sizeof(unsigned int) * partner_layers.size() + sizeof(unsigned int);
}

bool mini_max::database::UncompFile::SkvFileLayerStruct::LoadFromFile(
    HANDLE h_file) {
  DWORD dw_bytes_read;
  if (!ReadFile(h_file, &layer_is_completed_and_in_file,
                sizeof(layer_is_completed_and_in_file), &dw_bytes_read, NULL))
    return false;
  if (!ReadFile(h_file, &layer_offset, sizeof(layer_offset), &dw_bytes_read,
                NULL))
    return false;
  if (!ReadFile(h_file, &knots_in_layer, sizeof(knots_in_layer), &dw_bytes_read,
                NULL))
    return false;
  if (!ReadFile(h_file, &num_won_states, sizeof(num_won_states), &dw_bytes_read,
                NULL))
    return false;
  if (!ReadFile(h_file, &num_lost_states, sizeof(num_lost_states),
                &dw_bytes_read, NULL))
    return false;
  if (!ReadFile(h_file, &num_drawn_states, sizeof(num_drawn_states),
                &dw_bytes_read, NULL))
    return false;
  if (!ReadFile(h_file, &num_invalid_states, sizeof(num_invalid_states),
                &dw_bytes_read, NULL))
    return false;
  if (!ReadFile(h_file, &size_in_bytes, sizeof(size_in_bytes), &dw_bytes_read,
                NULL))
    return false;
  if (!LoadVectorFromFile(h_file, succ_layers)) return false;
  if (!LoadVectorFromFile(h_file, partner_layers)) return false;
  return true;
}

bool mini_max::database::UncompFile::SkvFileLayerStruct::SaveToFile(
    HANDLE h_file) const {
  DWORD dw_bytes_written;
  if (!WriteFile(h_file, &layer_is_completed_and_in_file,
                 sizeof(layer_is_completed_and_in_file), &dw_bytes_written,
                 NULL))
    return false;
  if (!WriteFile(h_file, &layer_offset, sizeof(layer_offset), &dw_bytes_written,
                 NULL))
    return false;
  if (!WriteFile(h_file, &knots_in_layer, sizeof(knots_in_layer),
                 &dw_bytes_written, NULL))
    return false;
  if (!WriteFile(h_file, &num_won_states, sizeof(num_won_states),
                 &dw_bytes_written, NULL))
    return false;
  if (!WriteFile(h_file, &num_lost_states, sizeof(num_lost_states),
                 &dw_bytes_written, NULL))
    return false;
  if (!WriteFile(h_file, &num_drawn_states, sizeof(num_drawn_states),
                 &dw_bytes_written, NULL))
    return false;
  if (!WriteFile(h_file, &num_invalid_states, sizeof(num_invalid_states),
                 &dw_bytes_written, NULL))
    return false;
  if (!WriteFile(h_file, &size_in_bytes, sizeof(size_in_bytes),
                 &dw_bytes_written, NULL))
    return false;
  if (!SaveVectorToFile(h_file, succ_layers)) return false;
  if (!SaveVectorToFile(h_file, partner_layers)) return false;
  return true;
}

bool mini_max::database::UncompFile::SkvFileLayerStruct::LoadVectorFromFile(HANDLE h_file, std::vector<unsigned int>& buffer) {
  DWORD dw_bytes_read;
  unsigned int bytes_to_read;
  if (!ReadFile(h_file, &bytes_to_read, sizeof(unsigned int), &dw_bytes_read,
                NULL))
    return false;
  buffer.resize(bytes_to_read / sizeof(unsigned int));
  return ReadFile(h_file, buffer.data(), bytes_to_read, &dw_bytes_read, NULL) &&
         dw_bytes_read == bytes_to_read;
}

bool mini_max::database::UncompFile::SkvFileLayerStruct::SaveVectorToFile(
    HANDLE h_file, const std::vector<unsigned int>& buffer) const {
  DWORD dw_bytes_written;
  unsigned int bytes_to_write = buffer.size() * sizeof(unsigned int);
  if (!WriteFile(h_file, &bytes_to_write, sizeof(unsigned int),
                 &dw_bytes_written, NULL))
    return false;
  if (!WriteFile(h_file, buffer.data(), bytes_to_write, &dw_bytes_written,
                 NULL))
    return false;
  return dw_bytes_written == bytes_to_write;
}

}  // namespace muehle