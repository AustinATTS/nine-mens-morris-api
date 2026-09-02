#include "muehle/mini_max/retro_analysis/successor_count_file_storage.h"

namespace muehle {

// Implementation goes here.

/* Constructor for the successor count file storage */
mini_max::retro_analysis::SuccessorCountFileStorage::SuccessorCountFileStorage(
    Logger& log, const std::wstring& file_directory,
    std::vector<LayerInfoStruct>& layers_to_calculate)
    : log(log), layers_to_calculate(layers_to_calculate) {
  std::wstringstream ss_count_array_path;
  ss_count_array_path << file_directory << (file_directory.size() ? "/" : "")
                      << "count_array";
  CreateDirectory(ss_count_array_path.str().c_str(), NULL);

  for (auto& layer : this->layers_to_calculate) {
    std::wstringstream ss_count_array_file_path;
    ss_count_array_file_path << ss_count_array_path.str() << "/count_array"
                             << layer.layer_number << ".dat";
    layer.s_count_array_file_path = ss_count_array_file_path.str();
    if ((layer.h_file_count_array =
             CreateFile(layer.s_count_array_file_path.c_str(),
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
      log << "ERROR: Could not open file "
          << layer.s_count_array_file_path.c_str() << "!\n";
      return;
    }
  }
}

/* Destructor for the successor count file storage */
mini_max::retro_analysis::SuccessorCountFileStorage::
    ~SuccessorCountFileStorage() {
  /* Close all file handles */
  for (auto& layer : layers_to_calculate) {
    if (layer.h_file_count_array != NULL) {
      CloseHandle(layer.h_file_count_array);
      layer.h_file_count_array = NULL;
    }
  }
}

/* Write the successor count arrays to file */
bool mini_max::retro_analysis::SuccessorCountFileStorage::Write() {
  /* Write data to file */
  for (auto& layer : layers_to_calculate) {
    DWORD dw_written;
    if (!WriteFile(layer.h_file_count_array,
                   layer.succ_count_array->succ_count_array.data(),
                   layer.num_knots_in_layer * sizeof(CountArrayVarType),
                   &dw_written, NULL)) {
      log << "ERROR: Could not write data to file "
          << layer.s_count_array_file_path.c_str() << "!\n";
      return ReturnValues::FalseOrStop();
    }

    /* Check if all data has been written */
    if (dw_written != layer.num_knots_in_layer * sizeof(CountArrayVarType)) {
      log << "ERROR: Could not write all data to file "
          << layer.s_count_array_file_path.c_str() << "!\n";
      return ReturnValues::FalseOrStop();
    }
    log << " Count array saved to file: "
        << layer.s_count_array_file_path.c_str() << "\n";
  }
  return true;
}

/* Read the successor count arrays from file, if they exist */
bool mini_max::retro_analysis::SuccessorCountFileStorage::Read() {
  /* Locals */
  DWORD dw_read;

  /* Do all files exist? if one is missing, then return false */
  for (auto& layer : layers_to_calculate) {
    LARGE_INTEGER file_size;
    if (layer.h_file_count_array != NULL ||
        layer.h_file_count_array == INVALID_HANDLE_VALUE) {
      return false;
    }
    if (!GetFileSizeEx(layer.h_file_count_array, &file_size) ||
        file_size.QuadPart !=
            layer.num_knots_in_layer * sizeof(CountArrayVarType)) {
      return false;
    }
  }

  /* Read data from file */
  for (auto& layer : layers_to_calculate) {
    log << " Load number of succedors from file: "
        << layer.s_count_array_file_path.c_str() << "\n";

    /* Read data from file */
    if (!ReadFile(layer.h_file_count_array,
                  layer.succ_count_array->succ_count_array.data(),
                  layer.num_knots_in_layer * sizeof(CountArrayVarType),
                  &dw_read, NULL)) {
      log << "ERROR: Could not read data from file "
          << layer.s_count_array_file_path.c_str() << "!\n";
      return ReturnValues::FalseOrStop();
    }

    /* Check if all data was read */
    if (dw_read != layer.num_knots_in_layer * sizeof(CountArrayVarType)) {
      log << "ERROR: Could not read all data from file "
          << layer.s_count_array_file_path.c_str() << "!\n";
      return ReturnValues::FalseOrStop();
    }
  }
  return true;
}

}  // namespace muehle
