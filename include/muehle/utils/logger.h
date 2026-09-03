#ifndef MUEHLE_UTILS_LOGGER_H_
#define MUEHLE_UTILS_LOGGER_H_

#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace muehle {

/* Logger class
 * - Logs messages to the console, a file or both
 * - The log level can be set to none, error, warning, info, debug, or trace
 * - The log type can be set to none, console, file, or both
 * - The filename is only used if the log type is set to file or both
 * - The logger can be used like a stream e.g., log << "message" << std::endl;
 * - Thread safe
 */
class Logger {
 public:
  /* Enumerations */
  enum class LogLevel { none, error, warning, info, debug, trace };

  enum class LogType { none, console, file, both };

  /* Constructors and destructor */
  Logger();
  Logger(LogLevel level, LogType type, const std::wstring& filename);
  Logger(Logger&& other) noexcept;
  ~Logger();

  /* Functions */
  bool SetOutputSteam(std::wostream& stream);
  bool SetLevel(LogLevel level);
  LogLevel GetLevel() const {
    return m_level;
  }
  LogType GetType() const {
    return m_type;
  }
  bool Log(LogLevel level, const std::wstring& message, bool new_line = true,
           bool begin = true);
  bool Log(LogLevel level, const std::wstring& message,
           const std::wstring& function, const std::wstring& file, int line);
  Logger& operator=(Logger&& other) noexcept;

  /* Steam operator */
  template <typename T>
  friend Logger& operator<<(Logger& log, const T& data) {
    std::wstringstream wss;
    wss << data;
    /* If we are at the beginning of a new line, then we need to log header
     * information */
    if (log.next_is_begin) {
      log.Log(LogLevel::info, /* message: */ wss.str(), /* new_line: */ false,
              /* begin: */ false);
      log.next_is_begin = false;
    } else {
      log.Log(LogLevel::info, /* message: */ wss.str(), /* new_line: */ false,
              /* begin: */ false);
    }
    /* If the string ends with a newline, then the next string will be at the
     * beginning of a new line */
    if (wss.str().ends_with(/* x: */ L"\n")) {
      log.next_is_begin = true;
    }
    return log;
  }

 private:
  LogLevel m_level = LogLevel::info;
  LogType m_type = LogType::console;
  std::wstring m_filename;
  std::wofstream m_file;
  std::mutex m_mutex;
  bool next_is_begin = true;
  std::wostream* stream = &std::wcout;
  bool destroyed = false;

  std::wstring GetLevelString(LogLevel level);
  std::wstring GetTimeString();
  std::wstring GetLogString(LogLevel level, const std::wstring& message,
                            const std::wstring& function,
                            const std::wstring& file, int line);
  bool GetReturnValue(LogLevel level);
};
} /* namespace muehle */

#endif /* MUEHLE_UTILS_LOGGER_H_ */
