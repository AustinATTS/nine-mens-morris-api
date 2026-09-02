#include "muehle/mini_max/database/comp_file.h"

#include "muehle/utils/logger.h"
#include "muehle/mini_max/database/generic_file.h"

namespace muehle {

mini_max::database::CompFile::CompFile(GameInterface* game, Logger& log)
    : GenericFile{game, log} {
  file.SetBlockSize(block_size_in_bytes);
}

void mini_max::database::CompFile::UpdateFileName() {
  std::wstringstream ss_database_file_path;
  ss_database_file_path << file_directory << (file_directory.size() ? "/" : "")
                     << L"database.dat";
  file_name = ss_database_file_path.str();
}

bool mini_max::database::CompFile::OpenDatabase(std::wstring const& file_directory) {
  log << "Open database from folder "
      << std::filesystem::absolute(file_directory) << "\n";
  this->file_directory = file_directory;
  UpdateFileName();
  if (!file.Open(file_name, false))
    return log.Log(Logger::LogLevel::error, L"Failed to open database file.");
  file_opened = true;
  return true;
}

void mini_max::database::CompFile::CloseDatabase() {
  log << "Close database.\n";
  file.Close();
  file_opened = false;

  // reset cache
  db_stats_cache.completed = false;
  db_stats_cache.num_layers = 0;
  layer_stats_cache.clear();
}

// Desc: Deletes the database file.
//       If 'fileDirectory' is omitted, the function will attempt to remove the
//       file at the current 'fileDirectory' path.
//-----------------------------------------------------------------------------
bool mini_max::database::CompFile::RemoveFile(std::wstring const& file_directory) {
  log << "Remove database file.\n";
  if (IsOpen()) CloseDatabase();
  this->file_directory = file_directory;
  UpdateFileName();
  _wremove(file_name.c_str());

  // return false, if file still exists
  if (std::filesystem::exists(file_name))
    return log.Log(Logger::LogLevel::error, L"Failed to remove database file.");

  log << "Database file " << file_name << " removed successfully.\n";
  this->file_directory = L"";
  file_opened = false;
  return true;
}

void mini_max::database::CompFile::UpdateCache(
    const DatabaseStatsStruct& db_stats,
    const std::vector<LayerStatsStruct>& layer_stats) {
  db_stats_cache = db_stats;
  layer_stats_cache.resize(layer_stats.size());
  for (size_t i = 0; i < layer_stats.size(); i++) {
    layer_stats_cache[i].completed_and_in_file = layer_stats[i].completed_and_in_file;
    layer_stats_cache[i].partner_layer = layer_stats[i].partner_layer;
    layer_stats_cache[i].knots_in_layer = layer_stats[i].knots_in_layer;
    layer_stats_cache[i].num_won_states = layer_stats[i].num_won_states;
    layer_stats_cache[i].num_lost_states = layer_stats[i].num_lost_states;
    layer_stats_cache[i].num_drawn_states = layer_stats[i].num_drawn_states;
    layer_stats_cache[i].num_invalid_states = layer_stats[i].num_invalid_states;
    layer_stats_cache[i].succ_layers = layer_stats[i].succ_layers;
    layer_stats_cache[i].partner_layers = layer_stats[i].partner_layers;
  }
}

bool mini_max::database::CompFile::ReadSection(const std::wstring& key,
                                              std::vector<unsigned int>& buffer) {
  if (!file_opened)
    return log.Log(Logger::LogLevel::error,
                   L"Cannot read section, since database is not open.");
  unsigned int bytes_to_read = file.GetSizeOfUncompressedSection(key);
  if (bytes_to_read == 0)
    return log.Log(Logger::LogLevel::error, L"Section does not exist.");
  buffer.resize(bytes_to_read / sizeof(unsigned int));
  return file.Read(key, 0, bytes_to_read, buffer.data());
}

bool mini_max::database::CompFile::LoadHeader(
    DatabaseStatsStruct& db_stats, std::vector<LayerStatsStruct>& layer_stats) {
  if (!file_opened)
    return log.Log(Logger::LogLevel::error,
                   L"Cannot load header, since database is not open.");
  log << "Load database header.\n";

  if (file.GetSizeOfCompressedSection(L"db_stats") > 0) {
    file.Read(L"db_stats", 0, sizeof(db_stats), &db_stats);
    layer_stats.resize(db_stats.num_layers);
    for (unsigned int i = 0; i < db_stats.num_layers; i++) {
      std::wstring base_key_name =
          std::wstring(L"layerStats") +
          std::to_wstring(i);  // remember the base key name for later use
      if (!file.Read(std::wstring(L"layer_stats") + std::to_wstring(i), 0,
                     LayerStatsStruct::num_bytes_layer_stats_header,
                     &layer_stats[i]))
        return false;
      if (!layer_stats[i].knots_in_layer) {
        continue;
      }
      if (!ReadSection(base_key_name + std::wstring(L".succLayers"),
                       layer_stats[i].succ_layers))
        return false;
      // only new databases have multiple partnerLayers
      if (file.DoesKeyExist(base_key_name + std::wstring(L".partnerLayers"))) {
        if (!ReadSection(base_key_name + std::wstring(L".partnerLayers"),
                         layer_stats[i].partner_layers))
          return false;
        // old databases have only one partnerLayer
      } else {
        layer_stats[i].partner_layers.assign({layer_stats[i].partner_layer});
      }
      layer_stats[i].skv.clear();
      layer_stats[i].ply_info.clear();
    }
  } else {
    db_stats.completed = false;
    db_stats.num_layers = game->GetNumberOfLayers();
    layer_stats.resize(db_stats.num_layers);
    for (unsigned int i = 0; i < db_stats.num_layers; i++) {
      game->GetSuccLayers(i, layer_stats[i].succ_layers);
      layer_stats[i].completed_and_in_file = false;
      layer_stats[i].knots_in_layer = game->GetNumberOfKnotsInLayer(i);
      layer_stats[i].partner_layers = game->GetPartnerLayers(i);
      layer_stats[i].partner_layer =
          layer_stats[i]
              .partner_layers[0];  // only one partner layer in legacy databases
      layer_stats[i].num_won_states = 0;
      layer_stats[i].num_lost_states = 0;
      layer_stats[i].num_drawn_states = 0;
      layer_stats[i].num_invalid_states = 0;
    }
  }
  UpdateCache(db_stats, layer_stats);
  return true;
}

bool mini_max::database::CompFile::SaveHeader(
    const DatabaseStatsStruct& db_stats,
    const std::vector<LayerStatsStruct>& layer_stats) {
  if (!IsOpen() || !file.IsOpen())
    return log.Log(Logger::LogLevel::error,
                   L"Cannot save header, since database is not open.");
  log << "Save database header.\n";
  if (!file.Write(L"db_stats", 0, sizeof(db_stats), &db_stats)) {
    return log.Log(Logger::LogLevel::error, L"Failed to save database stats.");
  }
  for (unsigned int i = 0; i < db_stats.num_layers; i++) {
    if (!file.Write(std::wstring(L"layer_stats") + std::to_wstring(i), 0,
                    LayerStatsStruct::num_bytes_layer_stats_header,
                    &layer_stats[i])) {
      return log.Log(Logger::LogLevel::error, L"Failed to save layer stats.");
    }
    if (layer_stats[i].succ_layers.size()) {
      if (!file.Write(
              std::wstring(L"layer_stats") + std::to_wstring(i) + std::wstring(L".succ_layers"),
              0, sizeof(unsigned int) * layer_stats[i].succ_layers.size(),
              layer_stats[i].succ_layers.data())) {
        return log.Log(Logger::LogLevel::error, L"Failed to save layer stats.");
      }
    }
    if (layer_stats[i].partner_layers.size()) {
      if (!file.Write(std::wstring(L"layer_stats") + std::to_wstring(i) +
                          std::wstring(L".partner_layers"),
                      0,
                      sizeof(unsigned int) * layer_stats[i].partner_layers.size(),
                      layer_stats[i].partner_layers.data())) {
        return log.Log(Logger::LogLevel::error, L"Failed to save layer stats.");
      }
    }
  }
  UpdateCache(db_stats, layer_stats);
  return true;
}

//-----------------------------------------------------------------------------
// Name: isOpen()
// Desc:
//-----------------------------------------------------------------------------
bool mini_max::database::CompFile::IsOpen() { return file_opened; }

//-----------------------------------------------------------------------------
// Name: readSkv()
// Desc:
//-----------------------------------------------------------------------------
bool mini_max::database::CompFile::ReadSkv(unsigned int layer_num,
                                          std::vector<TwoBit>& skv) {
  if (!IsOpen())
    return log.Log(Logger::LogLevel::error,
                   L"Cannot read skv, since database is not open.");
  if (layer_num >= layer_stats_cache.size())
    return log.Log(Logger::LogLevel::error, L"Layer number out of range.");
  if (!layer_stats_cache[layer_num].completed_and_in_file)
    return log.Log(Logger::LogLevel::error, L"Layer is not in file.");
  if (skv.size() != layer_stats_cache[layer_num].GetLayerSizeInBytesForSkv())
    return log.Log(Logger::LogLevel::error,
                   L"Size of passed vector does not match size of layer.");
  if (!file.Read(std::wstring(L"skv") + std::to_wstring(layer_num), 0,
                 skv.size() * sizeof(TwoBit), &skv[0]))
    return log.Log(Logger::LogLevel::error, L"Failed to read skv.");
  return true;
}

//-----------------------------------------------------------------------------
// Name: readSkv()
// Desc:
//-----------------------------------------------------------------------------
bool mini_max::database::CompFile::ReadSkv(unsigned int layer_num,
                                          TwoBit& database_byte,
                                          unsigned int state_number) {
  if (!IsOpen())
    return log.Log(Logger::LogLevel::error,
                   L"Cannot read skv, since database is not open.");
  if (layer_num >= layer_stats_cache.size())
    return log.Log(Logger::LogLevel::error, L"Layer number out of range.");
  if (!layer_stats_cache[layer_num].completed_and_in_file)
    return log.Log(Logger::LogLevel::error, L"Layer is not in file.");
  if (state_number >= layer_stats_cache[layer_num].knots_in_layer)
    return log.Log(Logger::LogLevel::error, L"State number out of range.");
  if (!file.Read(std::wstring(L"skv") + std::to_wstring(layer_num),
                 (state_number / 4) * sizeof(TwoBit), 1, &database_byte))
    return log.Log(Logger::LogLevel::error, L"Failed to read skv.");
  return true;
}

//-----------------------------------------------------------------------------
// Name: writeSkv()
// Desc:
//-----------------------------------------------------------------------------
bool mini_max::database::CompFile::WriteSkv(unsigned int layer_num,
                                           const std::vector<TwoBit>& skv) {
  if (!IsOpen())
    return log.Log(Logger::LogLevel::error,
                   L"Cannot read skv, since database is not open.");
  if (layer_num >= layer_stats_cache.size())
    return log.Log(Logger::LogLevel::error, L"Layer number out of range.");
  if (skv.size() != layer_stats_cache[layer_num].GetLayerSizeInBytesForSkv())
    return log.Log(Logger::LogLevel::error,
                   L"Size of passed vector does not match size of layer.");
  if (!file.Write(std::wstring(L"skv") + std::to_wstring(layer_num), 0,
                  skv.size() * sizeof(TwoBit), &skv[0]))
    return log.Log(Logger::LogLevel::error, L"Failed to write skv.");
  layer_stats_cache[layer_num].completed_and_in_file = true;
  return true;
}

//-----------------------------------------------------------------------------
// Name: readPlyInfo()
// Desc:
//-----------------------------------------------------------------------------
bool mini_max::database::CompFile::ReadPlyInfo(unsigned int layer_num,
                                              std::vector<PlyInfoVarType>& ply_info) {
  if (!IsOpen())
    return log.Log(Logger::LogLevel::error,
                   L"Cannot read skv, since database is not open.");
  if (layer_num >= layer_stats_cache.size())
    return log.Log(Logger::LogLevel::error, L"Layer number out of range.");
  if (!layer_stats_cache[layer_num].completed_and_in_file)
    return log.Log(Logger::LogLevel::error, L"Layer is not in file.");
  if (ply_info.size() != layer_stats_cache[layer_num].knots_in_layer)
    return log.Log(Logger::LogLevel::error,
                   L"Size of passed vector does not match size of layer.");
  if (!file.Read(std::wstring(L"plyInfo") + std::to_wstring(layer_num), 0,
                 ply_info.size() * sizeof(PlyInfoVarType), &ply_info[0]))
    return log.Log(Logger::LogLevel::error, L"Failed to read ply info.");
  return true;
}

//-----------------------------------------------------------------------------
// Name: readPlyInfo()
// Desc:
//-----------------------------------------------------------------------------
bool mini_max::database::CompFile::ReadPlyInfo(unsigned int layer_num,
                                              PlyInfoVarType& single_ply_info,
                                              unsigned int state_number) {
  if (!IsOpen())
    return log.Log(Logger::LogLevel::error,
                   L"Cannot read skv, since database is not open.");
  if (layer_num >= layer_stats_cache.size())
    return log.Log(Logger::LogLevel::error, L"Layer number out of range.");
  if (!layer_stats_cache[layer_num].completed_and_in_file)
    return log.Log(Logger::LogLevel::error, L"Layer is not in file.");
  if (state_number >= layer_stats_cache[layer_num].knots_in_layer)
    return log.Log(Logger::LogLevel::error, L"State number out of range.");
  if (!file.Read(std::wstring(L"plyInfo") + std::to_wstring(layer_num),
                 state_number * sizeof(PlyInfoVarType), sizeof(PlyInfoVarType),
                 &single_ply_info))
    return log.Log(Logger::LogLevel::error, L"Failed to read ply info.");
  return true;
}

//-----------------------------------------------------------------------------
// Name: writePlyInfo()
// Desc:
//-----------------------------------------------------------------------------
bool mini_max::database::CompFile::WritePlyInfo(
    unsigned int layer_num, const std::vector<PlyInfoVarType>& ply_info) {
  if (!IsOpen())
    return log.Log(Logger::LogLevel::error,
                   L"Cannot read skv, since database is not open.");
  if (layer_num >= layer_stats_cache.size())
    return log.Log(Logger::LogLevel::error, L"Layer number out of range.");
  if (!layer_stats_cache[layer_num].completed_and_in_file)
    return log.Log(Logger::LogLevel::error, L"Layer is not in file.");
  if (ply_info.size() != layer_stats_cache[layer_num].knots_in_layer)
    return log.Log(Logger::LogLevel::error,
                   L"Size of passed vector does not match size of layer.");
  if (!file.Write(std::wstring(L"plyInfo") + std::to_wstring(layer_num), 0,
                  ply_info.size() * sizeof(PlyInfoVarType), &ply_info[0]))
    return log.Log(Logger::LogLevel::error, L"Failed to write ply info.");
  layer_stats_cache[layer_num].completed_and_in_file = true;
  return true;
}

}