/* Minimal header file to act as a compatibility layer while porting over the
 * original code to compile and run on Linux systems. */

#ifndef MUEHLE_WIN_32_COMPAT_H_
#define MUEHLE_WIN_32_COMPAT_H_

#ifndef _WIN32

#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>

namespace muehle {

/* Basic Types */
using BOOL = int;
using DWORD = uint32_t;
using LONG = int32_t;
using LONGLONG = int64_t;
using WCHAR = wchar_t;
using LPVOID = void*;
using LPCVOID = const void*;
using SIZE_T = size_t;
using LPCWSTR = const wchar_t*;
using LPWSTR = wchar_t*;

#ifndef TRUE
#define TRUE 1
#endif  // TRUE
#ifndef FALSE
#define FALSE 0
#endif  // FALSE

#ifndef WINAPI
#define WINAPI
#endif  // WINAPI

/* Large Integer */
union LARGE_INTEGER {
  struct {
    uint32_t LowPart;
    uint32_t HighPart;
  };
  int64_t QuadPart;
};

/* File Constants */
constexpr DWORD GENERIC_READ = 0x80000000;
constexpr DWORD GENERIC_WRITE = 0x40000000;
constexpr DWORD FILE_SHARE_READ = 0x00000001;
constexpr DWORD FILE_SHARE_WRITE = 0x00000002;
constexpr DWORD OPEN_ALWAYS = 4;
constexpr DWORD OPEN_EXISTING = 3;
constexpr DWORD FILE_ATTRIBUTE_NORMAL = 0x80;
constexpr DWORD FILE_BEGIN = 0;
constexpr DWORD FILE_CURRENT = 1;
constexpr DWORD FILE_END = 2;
constexpr DWORD INFINITE = 0xFFFFFFFFu;
constexpr DWORD INVALID_SET_FILE_POINTER = 0xFFFFFFFFu;
constexpr int THREAD_PRIORITY_BELOW_NORMAL = -1;
constexpr DWORD CREATE_SUSPENDED = 0x00000004;

/* HANDLE file handle, thread handle, or process pseudo-handle */
namespace win_32_compat {
struct ThreadSuspendState {
  sem_t resume_sem;

  ThreadSuspendState() { sem_init(&resume_sem, 0, 0); }
  ~ThreadSuspendState() { sem_destroy(&resume_sem); }
};

inline thread_local ThreadSuspendState* tl_my_suspend_state = nullptr;
inline thread_local DWORD tl_thread_id = 0;
inline std::atomic<DWORD> g_next_thread_id{1};

/* SIGUSR1 is used to interrupt a thread at it's current point of execution and
 * have it block until resumed. This mirrors Win32's
 * SuspendThread()/ResumeThread() semantics enough for this code base, which
 * pairs a Suspend with a later Resume for the same thread. */
inline void SuspendSignalHandler(int) {
  if (tl_my_suspend_state != nullptr) {
    sem_post(&tl_my_suspend_state->resume_sem);
  }
}

inline void InstallSuspendHandlerOnce() {
  static std::once_flag flag;
  std::call_once(flag, []() {
    struct sigaction sa{};
    sa.sa_handler = SuspendSignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, nullptr);
  });
}

inline std::string ToNativePath(const wchar_t* wpath) {
  /* Re uses std::filesystem's wchar_t -> native string conversion, which is
   * already used elsewhere in the code. */
  return std::filesystem::path(wpath).string();
}
}  // namespace win_32_compat

struct HANDLE__ {
  int fd = -1; /* File descriptor for file handles */
  bool is_process_pseudo =
      false; /* True for the pseudo handle returned by GetCurrentProcess() */
  std::thread* thr = nullptr; /* For thread handles */
  pthread_t pthread_handle{};
  win_32_compat::ThreadSuspendState* suspend_state = nullptr;
  DWORD thread_id = 0;
};
using HANDLE = HANDLE__*;

#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)

/* File I/O */
inline HANDLE CreateFile(const wchar_t* path, DWORD access, DWORD /* share */,
                         void* /* sa */, DWORD creation, DWORD /* flags */,
                         void* /* templ */) {
  std::string native_path = win_32_compat::ToNativePath(path);

  int posix_flags;
  bool want_read = (access & GENERIC_READ) != 0;
  bool want_write = (access & GENERIC_WRITE) != 0;
  if (want_read && want_write) {
    posix_flags = O_RDWR;
  } else if (want_write) {
    posix_flags = O_WRONLY;
  } else {
    posix_flags = O_RDONLY;
  }
  if (creation == OPEN_ALWAYS) {
    posix_flags |= O_CREAT;
  }

  int fd = ::open(native_path.c_str(), posix_flags, 0644);
  if (fd < 0) {
    return INVALID_HANDLE_VALUE;
  }

  HANDLE h = new HANDLE__();
  h->fd = fd;
  return h;
}

inline BOOL ReadFile(HANDLE h, void* buf, DWORD num_bytes_to_read,
                     DWORD* num_bytes_read, void* /* overlapped */) {
  if (!h || h == INVALID_HANDLE_VALUE) {
    return FALSE;
  }
  ssize_t n = ::read(h->fd, buf, num_bytes_to_read);
  if (n < 0) {
    if (num_bytes_read) {
      *num_bytes_read = 0;
    }
    return FALSE;
  }
  if (num_bytes_read) {
    *num_bytes_read = (DWORD)n;
  }
  return TRUE;
}

inline BOOL WriteFile(HANDLE h, const void* buf, DWORD num_bytes_to_write,
                      DWORD* num_bytes_written, void* /* overlapped */) {
  if (!h || h == INVALID_HANDLE_VALUE) {
    return FALSE;
  }
  ssize_t n = ::write(h->fd, buf, num_bytes_to_write);
  if (n < 0) {
    if (num_bytes_written) {
      *num_bytes_written = 0;
    }
    return FALSE;
  }
  if (num_bytes_written) {
    *num_bytes_written = (DWORD)n;
  }
  return TRUE;
}

inline BOOL SetFilePointerEx(HANDLE h, LARGE_INTEGER distance,
                             LARGE_INTEGER* new_pos, DWORD method) {
  if (!h || h == INVALID_HANDLE_VALUE) {
    return FALSE;
  }
  int whence = (method == FILE_BEGIN)     ? SEEK_SET
               : (method == FILE_CURRENT) ? SEEK_CUR
                                          : SEEK_END;
  off_t res = ::lseek(h->fd, (off_t)distance.QuadPart, whence);
  if (res == (off_t)-1) {
    return FALSE;
  }
  if (new_pos) {
    new_pos->QuadPart = res;
  }
  return TRUE;
}

inline DWORD SetFilePointer(HANDLE h, long distance, long* /* distance_high */,
                            DWORD method) {
  if (!h || h == INVALID_HANDLE_VALUE) {
    return 0xFFFFFFFFu;
  }
  int whence = (method == FILE_BEGIN)
                   ? SEEK_SET
                   : (method == FILE_CURRENT ? SEEK_CUR : SEEK_END);
  off_t res = ::lseek(h->fd, (off_t)distance, whence);
  return (DWORD)res;
}

inline BOOL GetFileSizeEx(HANDLE h, LARGE_INTEGER* size) {
  if (!h || h == INVALID_HANDLE_VALUE) {
    return FALSE;
  }
  struct stat st{};
  if (::fstat(h->fd, &st) != 0) {
    return FALSE;
  }
  size->QuadPart = st.st_size;
  return TRUE;
}

inline BOOL CloseHandle(HANDLE h) {
  if (!h || h == INVALID_HANDLE_VALUE) {
    return FALSE;
  }
  if (h->is_process_pseudo) {
    return TRUE; /* Singleton, never deleted */
  }

  if (h->fd > 0) ::close(h->fd);
  if (h->thr) {
    if (h->thr->joinable()) {
      h->thr->join();
    }
    delete h->thr;
  }
  delete h->suspend_state;
  delete h;
  return TRUE;
}

inline BOOL CreateDirectory(LPCWSTR path, LPVOID /* security_attributes */) {
  std::error_code ec;
  std::filesystem::create_directories(std::filesystem::path(path), ec);
  return ec ? FALSE : TRUE;
}

/* <SVC intrinsic to rotate an 8-bit value right by 'shift' bits */
inline unsigned char _rotr8(unsigned char value, unsigned char shift) {
  shift &= 7;
  if (shift == 0) {
    return value;
  }
  return (unsigned char)((value >> shift) | (value << (8 - shift)));
}

constexpr DWORD INVALID_FILE_ATTRIBUTES = 0xFFFFFFFFu;
inline DWORD GetFileAttributes(LPCWSTR path) {
  std::error_code ec;
  bool exists = std::filesystem::exists(std::filesystem::path(path), ec);
  return (exists && !ec) ? 0u : INVALID_FILE_ATTRIBUTES;
}

inline int _wremove(LPCWSTR path) {
  std::error_code ec;
  bool removed = std::filesystem::remove(std::filesystem::path(path), ec);
  return (removed && !ec) ? 0 : -1;
}

inline DWORD GetFileSize(
    HANDLE h_file,
    DWORD* /* high_part, unsupported. Files here are always < 4GB */) {
  LARGE_INTEGER size;
  if (!GetFileSizeEx(h_file, &size)) {
    return INVALID_SET_FILE_POINTER;
  }
  return (DWORD)size.QuadPart;
}

struct MEMORYSTATUSEX {
  DWORD dwLength = 0;
  DWORD dwMemoryLoad = 0;
  uint64_t ullTotalPhys = 0;
  uint64_t ullAvailPhys = 0;
  uint64_t ullTotalPageFile = 0;
  uint64_t ullAvailPageFile = 0;
  uint64_t ullTotalVirtual = 0;
  uint64_t ullAvailVirtual = 0;
  uint64_t ullAvailExtendedVirtual = 0;
};

inline BOOL GlobalMemoryStatusEx(MEMORYSTATUSEX* status) {
  struct sysinfo si{};
  if (::sysinfo(&si) != 0) {
    return FALSE;
  }

  uint64_t total_phys = (uint64_t)si.totalram * si.mem_unit;
  uint64_t avail_phys = (uint64_t)si.freeram * si.mem_unit;
  uint64_t total_swap = (uint64_t)si.totalswap * si.mem_unit;
  uint64_t avail_swap = (uint64_t)si.freeswap * si.mem_unit;

  status->dwMemoryLoad =
      total_phys ? (DWORD)(100 - (avail_phys * 100) / total_phys) : 0;
  status->ullTotalPhys = total_phys;
  status->ullAvailPhys = avail_phys;
  status->ullTotalPageFile = total_phys + total_swap;
  status->ullAvailPageFile = avail_phys + avail_swap;
  status->ullTotalVirtual = status->ullTotalPageFile;
  status->ullAvailVirtual = status->ullAvailPageFile;
  status->ullAvailExtendedVirtual = 0;
  return TRUE;
}

/* Critical sections */
struct CRITICAL_SECTION {
  std::recursive_mutex m;
};
inline void InitializeCriticalSection(CRITICAL_SECTION*) {}
inline void DeleteCriticalSection(CRITICAL_SECTION*) {}
inline void EnterCriticalSection(CRITICAL_SECTION* cs) { cs->m.lock(); }
inline void LeaveCriticalSection(CRITICAL_SECTION* cs) { cs->m.unlock(); }

/* Sleep / High resolution timers */
inline void Sleep(DWORD milliseconds) {
  std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

inline BOOL QueryPerformanceFrequency(LARGE_INTEGER* freq) {
  freq->QuadPart = 1000000000LL; /* Counts in nanoseconds */
  return TRUE;
}

inline BOOL QueryPerformanceCounter(LARGE_INTEGER* counter) {
  counter->QuadPart = std::chrono::duration_cast<std::chrono::nanoseconds>(
                          std::chrono::steady_clock::now().time_since_epoch())
                          .count();
  return TRUE;
}

/* Threads */
using LPTHREAD_START_ROUTINE = DWORD (*)(LPVOID);

struct SYSTEM_INFO {
  unsigned int dw_number_of_processors = 1;
};
inline void GetSystemInfo(SYSTEM_INFO* si) {
  unsigned int n = std::thread::hardware_concurrency();
  si->dw_number_of_processors = (n == 0) ? 1 : n;
}

inline DWORD GetCurrentThreadId() { return win_32_compat::tl_thread_id; }

inline HANDLE CreateThread(void* /* sa */, SIZE_T /* stack_size */,
                           LPTHREAD_START_ROUTINE start_addr, LPVOID param,
                           DWORD creation_flags, DWORD* thread_id_out) {
  HANDLE h = new HANDLE__();
  h->suspend_state = new win_32_compat::ThreadSuspendState();

  DWORD new_id = win_32_compat::g_next_thread_id.fetch_add(1);
  h->thread_id = new_id;
  if (thread_id_out) {
    *thread_id_out = new_id;
  }

  bool start_suspended = (creation_flags & CREATE_SUSPENDED) != 0;

  h->thr = new std::thread([=]() {
    win_32_compat::tl_thread_id = new_id;
    win_32_compat::tl_my_suspend_state = h->suspend_state;
    win_32_compat::InstallSuspendHandlerOnce();
    if (start_suspended) {
      sem_wait(&h->suspend_state->resume_sem);
    }
    start_addr(param);
  });
  h->pthread_handle = h->thr->native_handle();
  return h;
}

inline BOOL SetThreadPriority(HANDLE, int) { return TRUE; }

inline DWORD SuspendThread(HANDLE h) {
  if (h && h != INVALID_HANDLE_VALUE) {
    pthread_kill(h->pthread_handle, SIGUSR1);
  }
  return 0;
}

inline DWORD ResumeThread(HANDLE h) {
  if (h && h != INVALID_HANDLE_VALUE) {
    sem_post(&h->suspend_state->resume_sem);
  }
  return 0;
}

inline DWORD WaitForSingleObject(HANDLE h, DWORD /* milliseconds */) {
  if (!h || h == INVALID_HANDLE_VALUE) {
    return 0xFFFFFFFFu;
  }
  if (h->is_process_pseudo) {
    /* used by returnValues::falseOrStop() to hang the program */
    for (;;) {
      std::this_thread::sleep_for(std::chrono::hours(1));
    }
  }
  if (h->thr && h->thr->joinable()) {
    h->thr->join();
  }
  return 0;
}

inline DWORD WaitForMultipleObjects(DWORD count, const HANDLE* handles,
                                    BOOL /* wait_all */,
                                    DWORD /* milliseconds */) {
  for (DWORD i = 0; i < count; i++) {
    if (handles[i] && handles[i] != INVALID_HANDLE_VALUE && handles[i]->thr &&
        handles[i]->thr->joinable()) {
      handles[i]->thr->join();
    }
  }
  return 0;
}

inline HANDLE GetCurrentProcess() {
  static HANDLE__ pseudo = [] {
    HANDLE__ h;
    h.is_process_pseudo = true;
    return h;
  }();
  return &pseudo;
}

}  // namespace muehle

#endif  // _WIN32

#endif  // MUEHLE_WIN_32_COMPAT_H_