#ifndef MUEHLE_UTILS_THREAD_MANAGER_CLASS_H_
#define MUEHLE_UTILS_THREAD_MANAGER_CLASS_H_

/* Standard library & win32 api*/
#ifdef _WIN32
#include <windows.h>
#else  // _WIN32
#include "muehle/win_32_compat.h"
#endif  // _WIN32
#include <barrier>
#include <cstdio>
#include <iostream>
#include <memory>
#include <vector>

namespace muehle {

#define TM_SCHEDULE_USER_DEFINED 0 /* User defined scheduling */
#define TM_SCHEDULE_STATIC \
  1 /* Each thread gets the same number of iterations */
#define TM_SCHEDULE_DYNAMIC 2   /* Not implemented yet */
#define TM_SCHEDULE_GUIDED 3    /* Not implemented yet */
#define TM_SCHEDULE_RUNTIME 4   /* Not implemented yet */
#define TM_SCHEDULE_NUM_TYPES 5 /* Number of scheduling types */

#define TM_RETURN_VALUE_OK 0 /* Return values of the execution functions */
#define TM_RETURN_VALUE_TERMINATE_ALL_THREADS 1
#define TM_RETURN_VALUE_EXECUTION_CANCELLED 2
#define TM_RETURN_VALUE_INVALID_PARAM 3
#define TM_RETURN_VALUE_UNEXPECTED_ERROR 4

/* In principle, the ThreadManagerClass is a wrapper for the win32 api functions
 * for creating and managing threads. The class provides a simple interface for
 * executing a function in parallel on multiple threads. It is possible to
 * execute a function in parallel on multiple threads, or to execute a loop in
 * parallel on multiple threads. The loop is divided into chunks and each thread
 * gets a chunk to work on. The class also provides a barrier function, which
 * can be used to synchronise threads. The class also provides a function to
 * pause and cancel the execution of the threads. The class also provides a
 * function to set a callback function which is called every x-milliseconds
 * during execution between two iterations. */
class ThreadManagerClass {
 private:
  /* Structures */

  /* Structure used in ThreadForLoop() */
  struct ForLoopStruct {
    unsigned int schedule_type =
        TM_SCHEDULE_USER_DEFINED; /* Type of scheduling, for load balancing */
    int64_t increment = 1;        /* Step size of the loop */
    int64_t initial_value = 0;    /* Initial value of the loop */
    int64_t final_value = 0;      /* Final value of the loop */
    void* p_parameter = nullptr;  /* Pointer to the user defined structure */
    DWORD (*thread_proc)(void* p_parameter, int64_t index) =
        nullptr; /* Pointer to the user function to be executed by the threads
                  */
    ThreadManagerClass*
        thread_manager; /* Pointer to the ThreadManagerClass object */
  };

  struct ThreadItem {
    HANDLE h_thread;        /* The thread handle given by the system */
    DWORD thread_id;        /* The thread ID given by the system */
    unsigned int thread_no; /* The thread number from 0 to num_threads-1 */

    ThreadItem();
    ~ThreadItem();
  };

  /* Variables */
  unsigned int num_threads = 0; /* Number of threads */
  bool terminate_all_threads =
      false; /* True when CancelExecution() was called */
  bool execution_paused =
      false; /* True if thread execution is currently paused */
  bool execution_cancelled = false; /* True when CancelExecution() was called */
  std::vector<ThreadItem> threads; /* Array of size 'num_threads' containing the
                                      thread handles, thread ids */
  std::barrier<>* p_barrier = nullptr; /* Pointer to a barrier object */
  bool any_thread_on_last_iteration =
      false; /* True if any thread is on the last iteration */

  /* Functions */
  static DWORD WINAPI ThreadForLoop(LPVOID lp_parameter);
  bool ResizeArrays();

 public:
  class ThreadVarsArrayItem {
   public:
    unsigned int cur_thread_no;

    virtual void DestroyElement() {}; /* Destroy the element */
    virtual void Reduce() {};         /* Merge the results of the threads */
  };

  template <class VarType>
  class ThreadVarsArray {
   public:
    unsigned int number_of_threads = 0; /* Number of threads */
    std::vector<VarType> item; /* Array of size 'number_of_threads' containing
                                  the ThreadVarsArrayItem objects */

    ThreadVarsArray(unsigned int number_of_threads, VarType const& master) {
      this->number_of_threads = number_of_threads;
      this->item = std::vector<VarType>(number_of_threads, master);

      for (unsigned int thread_counter = 0; thread_counter < number_of_threads;
           thread_counter++) {
        item[thread_counter].cur_thread_no = thread_counter;
      }
    };

    ~ThreadVarsArray() {
      for (unsigned int thread_counter = 0; thread_counter < number_of_threads;
           thread_counter++) {
        item[thread_counter].DestroyElement();
      }
    };

    void* GetPointerToArray() { return (void*)item.data(); };

    unsigned int GetSizeOfArray() { return sizeof(VarType); };

    void Reduce() {
      for (unsigned int thread_counter = 0; thread_counter < number_of_threads;
           thread_counter++) {
        item[thread_counter].Reduce();
      }
    };
  };

  /* Constructor / Descructor */
  ThreadManagerClass();
  ~ThreadManagerClass();

  /* Functions */
  unsigned int
  GetThreadNumber();            /* Returns a number from 0 to 'num_threads'-1 */
  unsigned int GetNumThreads(); /* Returns the total number of threads */

  bool SetNumThreads(
      unsigned int
          new_num_threads); /* Tries to set the number of threads. Returns false
                               if any thread is running. */

  void WaitForOtherThreads(); /* Waits for all threads to reach this point */
  void WaitForAllThreadsToTerminate(); /* Waits for all threads to terminate */
  void PauseExecution();               /* un-/suspend all threads */
  void CancelExecution();              /* terminate_all_threads = true */
  bool WasExecutionCancelled(); /* Tells if the execution was cancelled */
  void Reset();                 /* Resets to the initial state */
  void SetCallBackFunction(
      void UserFunction(void* p_user), void* p_user,
      DWORD
          milliseconds); /* A user function which is called every x-milliseconds
                            during execution between two iterations */

  bool AnyThreadRunning(); /* Returns true if any thread is running */

  /* Execute */
  unsigned int ExecuteInParallel(DWORD ThreadProc(void* p_parameter),
                                 void* p_parameter,
                                 unsigned int parameter_struct_size);
  unsigned int ExecuteParallelLoop(
      DWORD ThreadProc(void* p_parameter, int64_t index), void* p_parameter,
      unsigned int parameter_struct_size, unsigned int schedule_type,
      int64_t initial_value, int64_t final_value, int64_t increment);
};

}  // namespace muehle

#endif  // MUEHLE_UTILS_THREAD_MANAGER_CLASS_H_
