#include "muehle/mini_max/alpha_beta/common_thread_vars.h"

#ifdef _MSC_VER
/* Prevent macro conflicts with min/max in Windows header */
#define NOMINMAX
#endif /* _MSC_VER */
/* For std::min */
#include <algorithm>

namespace muehle {

std::mutex mini_max::CommonThreadVars::buffer_mutex = {};

bool mini_max::CommonThreadVars::LoadDataToBuffer() {
  /* Check if file is open */
  if (h_file == NULL || h_file == INVALID_HANDLE_VALUE) {
    return log.Log(
        Logger::LogLevel::error,
        /* messag: */L"File not open for reading. Loading data to buffer failed!");
  }

  /* Read data from file */
  buffer.clear();
  buffer_position = 0;
  unsigned int bytes_to_load = std::min(
      static_cast<unsigned int>(file_size - buffer_offset), max_buffer_size);
  buffer.resize(bytes_to_load);

  DWORD bytes_read = 0;
  {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    SetFilePointer(h_file, buffer_offset, NULL, FILE_BEGIN);
    if (ReadFile(h_file, &buffer[0], buffer.size(), &bytes_read, NULL) ==
        FALSE) {
      return log.Log(Logger::LogLevel::error,
                     L"ReadFile() failed at position " +
                         std::to_wstring(buffer_offset) + L"!");
    }
  }

  return true;
}

bool mini_max::CommonThreadVars::Flush() {
  /* Check if file is open */
  if (h_file == NULL || h_file == INVALID_HANDLE_VALUE) {
    return log.Log(Logger::LogLevel::error,
                   L"File not open for writing. Flushing failed!");
  }

  /* Write data to file */
  if (buffer.size()) {
    DWORD bytes_written = 0;
    std::lock_guard<std::mutex> lock(buffer_mutex);
    SetFilePointer(h_file, buffer_offset, NULL, FILE_BEGIN);
    WriteFile(h_file, &buffer[0], buffer.size(), &bytes_written, NULL);
    if (bytes_written != buffer.size()) {
      return log.Log(Logger::LogLevel::error,
                     L"WriteFile() failed at position " +
                         std::to_wstring(buffer_offset) + L"!");
    }
  }

  buffer.clear();
  buffer_position = 0;
  return true;
}

/* Constructor */
mini_max::CommonThreadVars::CommonThreadVars(CommonThreadVars const& master)
    : layer_number(master.layer_number),
      total_num_states_processed(master.total_num_states_processed),
      load_from_file(master.load_from_file),
      states_processed(master.states_processed),
      file_path(master.file_path),
      file_size(master.file_size),
      target_file_size(master.target_file_size),
      log(master.log) {
  if (file_path.empty()) {
    log.Log(Logger::LogLevel::trace,
            L"File path is empty. No file will be opened.");
    return;
  }

  buffer.reserve(max_buffer_size);

  /* Each thread needs its own file handle */
  if (load_from_file) {
    h_file = CreateFile(file_path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  } else {
    h_file = CreateFile(file_path.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE,
                        NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  }
  if (h_file == INVALID_HANDLE_VALUE) {
    h_file = NULL;
    load_from_file = false;
    log.Log(Logger::LogLevel::error,
            L"File handle is null. Failed to open file: " + file_path);
  }
}

/* Constructor */
mini_max::CommonThreadVars::CommonThreadVars(
    unsigned int layer_number, const std::wstring& file_path,
    int64_t target_file_size, int64_t& rough_total_num_states_processed,
    int64_t& total_num_states_processed, Logger& log)
    : layer_number(layer_number),
      total_num_states_processed(total_num_states_processed),
      states_processed(rough_total_num_states_processed),
      target_file_size(target_file_size),
      file_path(file_path),
      log(log) {
  if (file_path.empty()) {
    log.Log(Logger::LogLevel::trace,
            L"File path is empty. No file will be opened.");
    return;
  }

  /* Try to open file */
  h_file = CreateFile(file_path.c_str(), GENERIC_READ, 0, NULL, OPEN_ALWAYS,
                      FILE_ATTRIBUTE_NORMAL, NULL);
  if (h_file == INVALID_HANDLE_VALUE) {
    log.Log(Logger::LogLevel::error,
            L"File handle is null. Failed to open fileL " + file_path);
    h_file = NULL;
  }

  if (h_file) {
    LARGE_INTEGER li_file_size;
    GetFileSizeEx(h_file, &li_file_size);
    file_size = li_file_size.QuadPart;
  }

  if (file_size == target_file_size) {
    log << " Loading init states from file: " << file_path << "\n";
    load_from_file = true;
  }

  /* Close file again */
  if (h_file != NULL && h_file != INVALID_HANDLE_VALUE) {
    CloseHandle(h_file);
  }
  h_file = NULL; /* Each thread needs its own file handle */
  buffer.reserve(max_buffer_size);
}

/* Called by the thread manager. Thus no locking necessary here. */
mini_max::CommonThreadVars::~CommonThreadVars() {
  /* Close file */
  if (h_file != NULL && h_file != INVALID_HANDLE_VALUE) {
    CloseHandle(h_file);
  }
}

bool mini_max::CommonThreadVars::ReadByte(int64_t position_in_file,
                                          unsigned char& data) {
  /* Checks */
  if (!load_from_file) {
    return log.Log(Logger::LogLevel::error, L"File not open for reading!");
  }
  if (h_file == NULL || h_file == INVALID_HANDLE_VALUE) {
    log.Log(Logger::LogLevel::error, L"File handle is null. Failed to read!");
  }
  if (position_in_file < 0) {
    return log.Log(Logger::LogLevel::error, L"Position in file is negative!");
  }
  if (position_in_file >= target_file_size) {
    return log.Log(Logger::LogLevel::error,
                   L"Position in file is out of range");
  }

  /* Reload data, if maximum buffer size reached */
  if (!buffer.size() || buffer_position >= max_buffer_size) {
    buffer_offset = position_in_file;
    buffer_position = 0;
    LoadDataToBuffer();
  }

  /* Read data from buffer */
  data = buffer[buffer_position];
  buffer_position += 1;
  file_position = position_in_file;

  return true;
}

bool mini_max::CommonThreadVars::WriteByte(int64_t position_in_file,
                                           unsigned char data) {
  /* Checks */
  if (load_from_file) {
    return log.Log(Logger::LogLevel::error,
                   L"File is open for reading! Writing not possible!");
  }
  if (h_file == NULL || h_file == INVALID_HANDLE_VALUE) {
    log.Log(Logger::LogLevel::error, L"File not open for writing!");
  }
  if (position_in_file < 0) {
    return log.Log(Logger::LogLevel::error, L"Position in file is negative!");
  }
  if (position_in_file >= target_file_size) {
    return log.Log(Logger::LogLevel::error,
                   L"Position in file is out of range");
  }

  /* Flush, if maximum buffer size reached */
  if (buffer_position >= max_buffer_size) {
    if (!Flush()) {
      return log.Log(Logger::LogLevel::error, L"Flush() failed!");
    }
  }

  /* When buffer is empty, then set offset to current file position */
  if (!buffer.size()) {
    buffer_offset = position_in_file;
    buffer_position = 0;
  }

  /* Assume that only one byte is written at a time */
  buffer.push_back(data);
  buffer_position += 1;
  file_position = position_in_file;

  return true;
}

/* Called by the thread manager to reduce the thread specific data to the main
 * thread .*/
void mini_max::CommonThreadVars::Reduce() {
  /* When init file was created new then save it now */
  if (!load_from_file && h_file != NULL && h_file != INVALID_HANDLE_VALUE) {
    if (!Flush()) {
      log.Log(Logger::LogLevel::error, L"Flush() failed!");
      return;
    }
    if (cur_thread_no == 0) {
      log << "Saved initialised states to file: " << file_path << "\n";
    }
  }

  total_num_states_processed +=
      states_processed.GetStatesProcessedByThisThread();
}
}  // namespace muehle
