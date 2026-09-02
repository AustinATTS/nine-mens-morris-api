#include "muehle/mini_max/statistics/monitor.h"

#include "muehle/mini_max/mini_max.h"
#ifdef _WIN32
#include <windows.h>
#else // _WIN32
#include <muehle/win_32_compat.h>
#endif // _WIN32

namespace muehle {

void mini_max::statistics::Monitor::ShowMemoryStatus() {
  MEMORYSTATUSEX mem_status;
  mem_status.dwLength = sizeof(mem_status);
  GlobalMemoryStatusEx(&mem_status);

  std::cout << std::endl
            << "dwMemoryLoad           : " << mem_status.dwMemoryLoad;
  std::cout << std::endl
            << "ullAvailExtendedVirtual: "
            << mem_status.ullAvailExtendedVirtual;
  std::cout << std::endl
            << "ullAvailPageFile       : " << mem_status.ullAvailPageFile;
  std::cout << std::endl
            << "ullAvailPhys           : " << mem_status.ullAvailPhys;
  std::cout << std::endl
            << "ullAvailVirtual        : " << mem_status.ullAvailVirtual;
  std::cout << std::endl
            << "ullTotalPageFile       : " << mem_status.ullTotalPageFile;
  std::cout << std::endl
            << "ullTotalPhys           : " << mem_status.ullTotalPhys;
  std::cout << std::endl
            << "ullTotalVirtual        : " << mem_status.ullTotalVirtual;
}

bool mini_max::statistics::Monitor::CalcLayerStatistics(
    const char* statistics_file_name) {
  /* Locals */
  StateAddressStruct cur_state;
  TwoBit cur_state_value;

  /* Database must be open */
  if (!db->IsOpen()) {
    return log.Log(Logger::LogLevel::error,
                   L"Database must be open to calculate statistics!");
  }

  /* Open statistics file */
  std::wofstream stat_file(statistics_file_name,
                           std::ios::out | std::ios::trunc);

  /* Opened file successfully? */
  if (!stat_file.is_open()) {
    return log.Log(Logger::LogLevel::error, L"Failed to open statistics file");
  }

  /* Headline */
  stat_file << L"layer number,";
  stat_file << L"white stones ";
  stat_file << L"black stones,";
  stat_file << L"won states,";
  stat_file << L"lost states,";
  stat_file << L"draw states,";
  stat_file << L"invalid states,";
  stat_file << L"total num states,";
  stat_file << L"num succeding layers,";
  stat_file << L"partner layer,";
  stat_file << L"size in bytes,";
  stat_file << L"succ_layers[0] ";
  stat_file << L"succ_layers[1],";
  stat_file << L"max num plies WON,";
  stat_file << L"max ply state number WON,";
  stat_file << L"max num plies LOST,";
  stat_file << L"max ply state number LOST\n";

  mm->SetCurrentActivity(Activity::calc_layer_stats);

  /* Calc and show statistics */
  for (cur_state.layer_number = 0; cur_state.layer_number < db->GetNumLayers();
       cur_state.layer_number++) {
    /* Status output */
    log << "Calculating statistics of layer: " << (int)cur_state.layer_number
        << "\n";

    /* Only calc sats of completed layers */
    if (!db->UpdateLayerStats(cur_state.layer_number)) {
      return log.Log(Logger::LogLevel::error,
                     L"Failed to update layer stats of layer: " +
                         std::to_wstring(cur_state.layer_number) + L"\n");
    }

    /* Add line */
    auto& succ_layers = db->GetSuccLayers(cur_state.layer_number);
    auto& partner_layers = db->GetPartnerLayers(cur_state.layer_number);
    stat_file << cur_state.layer_number << L","
              << mm->game->GetOutputInformation(cur_state.layer_number) << L","
              << db->GetNumWonStates(cur_state.layer_number) << L","
              << db->GetNumLostStates(cur_state.layer_number) << L","
              << db->GetNumDrawnStates(cur_state.layer_number) << L","
              << db->GetNumInvalidStates(cur_state.layer_number) << L","
              << db->GetNumberOfKnots(cur_state.layer_number) << L","
              << (unsigned int)succ_layers.size() << L",";
    for (auto& partner_layer : partner_layers) {
      stat_file << partner_layer << L" ";
    }
    stat_file << L","
              << (unsigned int)db->GetLayerSizeInBytes(cur_state.layer_number)
              << L",";
    for (auto& succ_layer : succ_layers) {
      stat_file << succ_layer << L" ";
    }
    /* Get the maximum ply info value in the layer and show it as well as the
     * state number */
    {
      PlyInfoVarType max_ply_info_won;
      StateNumberVarType max_ply_state_won;
      PlyInfoVarType max_ply_info_lost;
      StateNumberVarType max_ply_state_lost;
      if (!CalcMaxPlyInfo(cur_state.layer_number, max_ply_info_won,
                          max_ply_state_won, max_ply_info_lost,
                          max_ply_state_lost)) {
        return log.Log(Logger::LogLevel::error,
                       L"Failed to calculate max ply info for layer: " +
                           std::to_wstring(cur_state.layer_number) + L"\n");
      }
      stat_file << L"," << max_ply_info_won << L"," << max_ply_state_won << L","
                << max_ply_info_lost << L"," << max_ply_state_lost;
    }
    stat_file << std::endl;

    /* Free memory */
    db->Unload();
  }

  /* Close file */
  stat_file.close();
  return true;
}

bool mini_max::statistics::Monitor::CalcMaxPlyInfo(
    unsigned int layer_number, PlyInfoVarType& max_ply_info_won,
    StateNumberVarType& max_ply_state_won, PlyInfoVarType& max_ply_info_lost,
    StateNumberVarType& max_ply_state_lost) {
  /* TODO: This function should be moved to
   * mini_max::database::Database::UpdateLayerStats() when the database file is
   * calculated the next time. */

  /* Locals */
  StateNumberVarType num_knots_in_layer = db->GetNumberOfKnots(layer_number);
  StateNumberVarType cur_state_number;
  TwoBit cur_state_value;
  PlyInfoVarType cur_ply_info;

  /* Init */
  max_ply_info_won = 0;
  max_ply_state_won = num_knots_in_layer;
  max_ply_info_lost = 0;
  max_ply_state_lost = num_knots_in_layer;

  /* Database layer must be fully loaded for better performance */
  db->SetLoadingOfFullLayerOnRead();

  /* Calc and show statistics */
  for (cur_state_number = 0; cur_state_number < num_knots_in_layer;
       cur_state_number++) {
    /* Get state value */
    if (!db->ReadKnotValueFromDatabase(layer_number, cur_state_number,
                                       cur_state_value)) {
      return log.Log(Logger::LogLevel::error,
                     L"ERROR: Reading knot value from database failed!");
    }

    if (cur_state_value == SKV_VALUE_GAME_WON ||
        cur_state_value == SKV_VALUE_GAME_LOST) {
      if (!db->ReadPlyInfoFromDatabase(layer_number, cur_state_number,
                                       cur_ply_info)) {
        return log.Log(Logger::LogLevel::error,
                       L"ERROR: Reading ply info from database failed!");
      }
    } else {
      continue;
    }

    /* Check if it is the maximum */
    if (cur_state_value == SKV_VALUE_GAME_WON) {
      if (cur_ply_info > max_ply_info_won) {
        max_ply_info_won = cur_ply_info;
        max_ply_state_won = cur_state_number;
      }
    } else if (cur_state_value == SKV_VALUE_GAME_LOST) {
      if (cur_ply_info > max_ply_info_lost) {
        max_ply_info_lost = cur_ply_info;
        max_ply_state_lost = cur_state_number;
      }
    }
  }
  return true;
}

/* Fills the 'layers' vector with the currently calculated layer and its partner
 * layers.
 * - Called by MAIN thread in p_mini_max->cs_os_print critical section.
 * - Not thread safe and must be called under external synchronisation */
void mini_max::statistics::Monitor::GetCurrentCalculatedLayer(
    std::vector<unsigned int>& layers) {
  layers.clear();
  layers.push_back(mm->cur_calculated_layer);
  for (auto i : db->GetPartnerLayers(mm->cur_calculated_layer)) {
    layers.push_back(i);
  }
}

/* Called by MAIN thread in p_mini_max->cs_os_print critical section */
const wchar_t* mini_max::statistics::Monitor::GetCurrentActionStr() {
  switch (cur_action) {
    case Activity::init_retro_analysis:
      return L"initiating retro analysis";
    case Activity::prepare_count_array:
      return L"preparing count arrays";
    case Activity::perform_retro_analysis:
      return L"performing retro analysis";
    case Activity::perform_alpha_beta:
      return L"performing alpha beta algorithmn";
    case Activity::testing_layer:
      return L"testing calculated layer";
    case Activity::saving_layer_to_file:
      return L"saving layer to file";
    case Activity::calc_layer_stats:
      return L"making layer statistics";
    case Activity::none:
      return L"none";
    default:
      return L"undefined";
  }
}

mini_max::statistics::Monitor::Monitor(MiniMax* p_mini_max, Logger& log)
    : log(log) {
  mm = p_mini_max;
  db = &p_mini_max->db;
}
}  // namespace muehle
