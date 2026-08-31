#include "muehle/utils/logger.h"

#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>

namespace muehle {

/* Constructor, specify log level, log type, and filename. */
Logger::Logger(LogLevel level, LogType type, const std::wstring& filename) {
  m_level = level;
  m_type = type;
  m_filename = filename;
  stream = &std::wcout;
  if (m_type == LogType::file || m_type == LogType::both) {
    if (m_filename.empty()) {
      throw std::runtime_error("Logger filename is empty.");
    }
    m_file.open(std::filesystem::path(m_filename),
                std::ios::out | std::ios::app);
    if (!m_file.is_open()) {
      throw std::runtime_error("Failed to open log file.");
    }
  }
}

/* Default constructor */
Logger::Logger() {
  m_level = LogLevel::info;
  m_type = LogType::console;
  stream = &std::wcout;
}

/* Move constructor */
Logger::Logger(Logger&& other) noexcept {
  m_level = other.m_level;
  m_type = other.m_type;
  m_filename = other.m_filename;
  m_file = std::move(other.m_file);
}

/* Move assignment operator */
Logger& Logger::operator=(Logger&& other) noexcept {
  if (this != &other) {
    m_level = other.m_level;
    m_type = other.m_type;
    m_filename = other.m_filename;
    m_file = std::move(other.m_file);
  }
  return *this;
}

/* Destructor */
Logger::~Logger() {
  /* The destroyer_flag is used to ensure that the log file is closed only once,
   * even if multiple logger instances are destroyed, preventing double-close
   * issues. */
  static std::atomic<bool> destroyed_flag{false};
  if (!destroyed_flag.exchange(true)) {
    if ((m_type == LogType::file || m_type == LogType::both) &&
        m_file.is_open()) {
      m_file.close();
    }
  }
  destroyed = true;
}

/* Set the output stream */
bool Logger::SetOutputSteam(std::wostream& stream) {
  if (!stream) {
    return false;
  }
  if (!stream.good()) {
    return false;
  }
  std::lock_guard<std::mutex> lock(m_mutex);
  this->stream = &stream;
  return true;
}

/* Set the log level */
bool Logger::SetLevel(LogLevel level) {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_level = level;
  return true;
}

/* Log a message */
bool Logger::Log(LogLevel level, const std::wstring& message, bool new_line,
                 bool begin) {
  if (destroyed) {
    return false;
  }
  if (level <= m_level) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_type == LogType::console || m_type == LogType::both) {
      *stream << (begin ? GetLogString(level, message, L"", L"", 0) : message)
              << (new_line ? L"\n" : L"") << std::flush;
    }

    if (m_type == LogType::file || m_type == LogType::both) {
      m_file << (begin ? GetLogString(level, message, L"", L"", 0) : message)
             << (new_line ? L"\n" : L"") << std::flush;
    }
  }
  return GetReturnValue(level);
}

/* Log a message with function, file and line information */
bool Logger::Log(LogLevel level, const std::wstring& message,
                 const std::wstring& function, const std::wstring& file,
                 int line) {
  if (destroyed) {
    return false;
  }
  if (level <= m_level) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_type == LogType::console || m_type == LogType::both) {
      *stream << GetLogString(level, message, function, file, line)
              << std::endl;
    }

    if (m_type == LogType::file || m_type == LogType::both) {
      m_file << GetLogString(level, message, function, file, line) << std::endl;
    }
  }
  return GetReturnValue(level);
}

/* Get the string representation of the Log Level */
std::wstring Logger::GetLevelString(LogLevel level) {
  switch (level) {
    case LogLevel::error:
      return L"ERROR";
    case LogLevel::warning:
      return L"WARN";
    case LogLevel::info:
      return L"INFO";
    case LogLevel::debug:
      return L"DEBUG";
    case LogLevel::trace:
      return L"TRACE";
    default:
      return L"";
  }
}

/* Get the current time as a string */
std::wstring Logger::GetTimeString() {
  std::time_t t = std::time(nullptr);
  std::tm tm;
#ifdef _WIN32
  localtime_s(&tm, &t);
#else
  localtime_r(&t, &tm);
#endif

  std::wstringstream ss;
  ss << std::put_time(&tm, L"%Y-%m-%d %H:%M:%S");
  return ss.str();
}

/* Get the log message as a string */
std::wstring Logger::GetLogString(LogLevel level, const std::wstring& message,
                                  const std::wstring& function,
                                  const std::wstring& file, int line) {
  std::wstringstream ss;
  ss << GetLevelString(level) << L" " << GetTimeString() << L" " << message;

  if (!function.empty()) {
    ss << L" (" << function << L" in " << file << L" at line " << line << L")";
  }

  return ss.str();
}

/* Get the return value for the log message */
bool Logger::GetReturnValue(LogLevel level) {
  switch (level) {
    case LogLevel::error:
      return false;
    case LogLevel::warning:
    case LogLevel::info:
    case LogLevel::debug:
    case LogLevel::trace:
      return true;
    default:
      return true;
  }
}

}  // namespace muehle
