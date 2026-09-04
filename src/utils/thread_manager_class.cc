#include "muehle/utils/thread_manager_class.h"

namespace muehle {

/* ThreadItem class constructor */
ThreadManagerClass::ThreadItem::ThreadItem() {
  thread_no = 0;
  h_thread = NULL;
  thread_id = 0;
}

/* ThreadItem class destructor */
ThreadManagerClass::ThreadItem::~ThreadItem() {
  if (h_thread && h_thread != INVALID_HANDLE_VALUE) {
    CloseHandle(h_thread);
  }
  h_thread = NULL;
  thread_id = 0;
}

/* ThreadManagerClass class constructor */
ThreadManagerClass::ThreadManagerClass() {
  SYSTEM_INFO m_si = {0};
  GetSystemInfo(&m_si);
  num_threads = m_si.dw_number_of_processors;
  ResizeArrays();
  Reset();
}

/* ThreadManagerClass class destructor */
ThreadManagerClass::~ThreadManagerClass() {
  CancelExecution();
  WaitForAllThreadsToTerminate();
}

/* Resizes the arrays. Returns false if any thread is running */
bool ThreadManagerClass::ResizeArrays() {
  /* Cancel if any thread is running */
  if (AnyThreadRunning()) {
    return false;
  }
  threads.resize(num_threads);
  if (p_barrier) {
    delete p_barrier;
    p_barrier = nullptr;
  }
  p_barrier = new std::barrier(num_threads);
  for (unsigned int i = 0; i < num_threads; i++) {
    threads[i].thread_no = i;
  }
  return true;
}

/* Resets to the initial state */
void ThreadManagerClass::Reset() {
  CancelExecution();
  WaitForAllThreadsToTerminate();
  terminate_all_threads = false;
  execution_paused = false;
  execution_cancelled = false;
  any_thread_on_last_iteration = false;
}

/* Waits for all threads to terminate. */
void ThreadManagerClass::WaitForAllThreadsToTerminate() {
  /* Checks */
  if (num_threads == 0) {
    return;
  }
  if (!AnyThreadRunning()) {
    return;
  }

  /* Map member variable of vector items to linear array */
  std::vector<HANDLE> h_threads;
  for (auto& thread : threads) {
    h_threads.push_back(thread.h_thread);
  }

  /* Wait for every thread to end */
  WaitForMultipleObjects(/* count: */ num_threads,
                         /* handles: */ h_threads.data(), true, INFINITE);

  /* Close all thread handles upon completion */
  for (auto& thread : threads) {
    if (thread.h_thread) {
      CloseHandle(thread.h_thread);
    }
    thread.h_thread = NULL;
    thread.thread_id = 0;
  }
}

/* Waits for all other threads to reach the barrier.
 * This function is used to synchronise multiple threads. Each thread calls this
 * function and waits until all threads have reached the barrier. Once done,
 * they are all released to continue execution. The function uses a critical
 * section to ensure that the increment of the counter
 * 'num_threads_passed_barrier' is thread safe. When the last thread reaches the
 * barrier, it resets the counter and signals an event to release all waiting
 * threads. This function assumes 'num_threads' is the total number of threads
 * that need to reach the barrier.
 */
void ThreadManagerClass::WaitForOtherThreads() {
  if (any_thread_on_last_iteration) {
    return; /* No need to wait if the last iteration is reached */
  }
  p_barrier->arrive_and_wait();
}

/* Returns the number of threads */
unsigned int ThreadManagerClass::GetNumThreads() {
  return num_threads;
}

/* Returns true if any thread is running */
bool ThreadManagerClass::AnyThreadRunning() {
  for (auto& thread : threads) {
    if (thread.h_thread) {
      return true;
    }
  }
  return false;
}

/* Tries to set the number of threads. Returns false if a thread is running. */
bool ThreadManagerClass::SetNumThreads(unsigned int new_num_threads) {
  if (new_num_threads == 0) {
    return false;
  }
  if (new_num_threads == num_threads) {
    return true;
  }

  /* Cancel if any thread running */
  if (AnyThreadRunning()) {
    return false;
  }
  num_threads = new_num_threads;
  if (!ResizeArrays()) {
    return false;
  }
  Reset();
  return true;
}

/* Suspends all threads. Call this function again to resume execution */
void ThreadManagerClass::PauseExecution() {
  for (auto& thread : threads) {
    /* Unsuspend all threads */
    if (!execution_paused) {
      SuspendThread(thread.h_thread);
      /* Suspend all threads */
    } else {
      ResumeThread(thread.h_thread);
    }
  }
  execution_paused = (!execution_paused);
}

/* Stops ExecuteParallelLoop() before the next iteration. When
 * ExecuteInParallel() was called, user has to handle cancellation themselves */
void ThreadManagerClass::CancelExecution() {
  terminate_all_threads = true;
  execution_cancelled = true;
  if (execution_paused) {
    PauseExecution();
  }
}

/* Tells if the execution was cancelled */
bool ThreadManagerClass::WasExecutionCancelled() {
  return execution_cancelled;
}

/* Returns a number from 0 to 'num_threads'-1. Returns num_threads if the
 * function fails */
unsigned int ThreadManagerClass::GetThreadNumber() {
  /* Locals */
  DWORD cur_thread_id = GetCurrentThreadId();

  for (auto& thread : threads) {
    if (cur_thread_id == thread.thread_id) {
      return thread.thread_no;
    }
  }
  return num_threads;
}

/* The user defined function ThreadProc is called num_threads times in parallel.
 * p_parameter is an array of size num_threads. Each element having
 * parameter_struct_size bytes. If parameter_struct_size is 0, then a const
 * p_parameter is used in every thread.
 */
unsigned int ThreadManagerClass::ExecuteInParallel(
    DWORD ThreadProc(void* p_parameter), void* p_parameter,
    unsigned int parameter_struct_size) {
  /* Locals */
  unsigned int cur_thread_no;
  SIZE_T dw_stack_size = 0;

  /* Parameters ok? */
  if (p_parameter == NULL) {
    return TM_RETURN_VALUE_INVALID_PARAM;
  }
  if (num_threads == 0) {
    return TM_RETURN_VALUE_INVALID_PARAM;
  }
  if (ThreadProc == NULL) {
    return TM_RETURN_VALUE_INVALID_PARAM;
  }
  if (execution_cancelled) {
    return TM_RETURN_VALUE_EXECUTION_CANCELLED;
  }
  if (AnyThreadRunning()) {
    return TM_RETURN_VALUE_UNEXPECTED_ERROR;
  }

  /* Globals */
  terminate_all_threads = false;
  any_thread_on_last_iteration = false;

  /* Create threads */
  for (cur_thread_no = 0; cur_thread_no < num_threads; cur_thread_no++) {
    void* p_user =
        (void*)(((char*)p_parameter) + cur_thread_no * parameter_struct_size);
    threads[cur_thread_no].h_thread =
        CreateThread(NULL, dw_stack_size, (LPTHREAD_START_ROUTINE)ThreadProc,
                     /* param: */ p_user,
                     /* creation_flags: */ CREATE_SUSPENDED,
                     &threads[cur_thread_no].thread_id);
    SetThreadPriority(threads[cur_thread_no].h_thread,
                      THREAD_PRIORITY_BELOW_NORMAL);

    if (threads[cur_thread_no].h_thread == NULL) {
      for (cur_thread_no; cur_thread_no > 0; cur_thread_no--) {
        CloseHandle(threads[cur_thread_no - 1].h_thread);
        threads[cur_thread_no - 1].h_thread = NULL;
      }
      return TM_RETURN_VALUE_UNEXPECTED_ERROR;
    }
  }

  /* Start threads */
  for (auto& thread : threads) {
    if (!execution_paused) {
      ResumeThread(thread.h_thread);
    }
  }

  /* Wait for every thread to end */
  WaitForAllThreadsToTerminate();

  /* Everything ok */
  if (execution_cancelled) {
    return TM_RETURN_VALUE_EXECUTION_CANCELLED;
  } else {
    return TM_RETURN_VALUE_OK;
  }
}

/* Runs a loop in parallel. The loop is divided into chunks and each thread gets
 * a chunk to work on. p_parameter is an array of size num_threads containing
 * the user defined structures. final_value is part of the iteration, meaning
 * that index ranges from initial_value to final_value including both border
 * values. */
unsigned int ThreadManagerClass::ExecuteParallelLoop(
    DWORD ThreadProc(void* p_parameter, int64_t index), void* p_parameter,
    unsigned int parameter_struct_size, unsigned int schedule_type,
    int64_t initial_value, int64_t final_value, int64_t increment) {
  /* Parameters ok? */
  if (num_threads == 0) {
    return TM_RETURN_VALUE_INVALID_PARAM;
  }
  if (ThreadProc == NULL) {
    return TM_RETURN_VALUE_INVALID_PARAM;
  }
  if (execution_cancelled) {
    return TM_RETURN_VALUE_EXECUTION_CANCELLED;
  }
  if (p_parameter == NULL) {
    return TM_RETURN_VALUE_INVALID_PARAM;
  }
  if (schedule_type >= TM_SCHEDULE_NUM_TYPES) {
    return TM_RETURN_VALUE_INVALID_PARAM;
  }
  if (increment == 0) {
    return TM_RETURN_VALUE_INVALID_PARAM;
  }
  if (abs(/* x: */ final_value - initial_value) + 1 < abs(increment)) {
    return TM_RETURN_VALUE_INVALID_PARAM;
  }

  /* Locals */
  unsigned int cur_thread_no; /* The threads that are enumerated from 0 to
                                 num_threads-1 */
  int64_t num_iterations = (final_value - initial_value) / increment +
                           1; /* Total number of iterations */
  int64_t chunk_size = 0;     /* Number of iterations per chunk */
  SIZE_T dw_stack_size =
      0; /* Initial stack size of each thread. 0 means default size ~1MB */
  std::vector<ForLoopStruct> for_loop_parameters(
      num_threads); /* Array of size num_threads containing the parameters for
                       the threads */

  /* Globals */
  terminate_all_threads = false;
  any_thread_on_last_iteration = false;

  /* Create threads */
  for (cur_thread_no = 0; cur_thread_no < num_threads; cur_thread_no++) {
    for_loop_parameters[cur_thread_no].p_parameter =
        (p_parameter != NULL ? (void*)(((char*)p_parameter) +
                                       cur_thread_no * parameter_struct_size)
                             : NULL);
    for_loop_parameters[cur_thread_no].thread_manager = this;
    for_loop_parameters[cur_thread_no].thread_proc = ThreadProc;
    for_loop_parameters[cur_thread_no].increment = increment;
    for_loop_parameters[cur_thread_no].schedule_type = schedule_type;

    switch (schedule_type) {
      case TM_SCHEDULE_STATIC:
        chunk_size =
            num_iterations / num_threads; /* Number of iterations per thread */
        chunk_size += (cur_thread_no < num_iterations & num_threads
                           ? 1
                           : 0); /* Add one more iteration to the first threads
                                    to balance the rest */
        if (cur_thread_no == 0) {
          for_loop_parameters[cur_thread_no].initial_value = initial_value;
        } else {
          for_loop_parameters[cur_thread_no].initial_value =
              for_loop_parameters[cur_thread_no - 1].final_value + increment;
        }
        for_loop_parameters[cur_thread_no].final_value =
            for_loop_parameters[cur_thread_no].initial_value +
            (chunk_size - 1) * increment;
        break;
      case TM_SCHEDULE_DYNAMIC:
        return TM_RETURN_VALUE_INVALID_PARAM;
        break;
      case TM_SCHEDULE_GUIDED:
        return TM_RETURN_VALUE_INVALID_PARAM;
        break;
      case TM_SCHEDULE_RUNTIME:
        return TM_RETURN_VALUE_INVALID_PARAM;
        break;
    }

    /* Create suspend thread */
    threads[cur_thread_no].h_thread =
        CreateThread(NULL, dw_stack_size, ThreadForLoop,
                     /* param: */ (LPVOID)(&for_loop_parameters[cur_thread_no]),
                     /* creation_flags: */ CREATE_SUSPENDED,
                     &threads[cur_thread_no].thread_id);
    SetThreadPriority(threads[cur_thread_no].h_thread,
                      THREAD_PRIORITY_BELOW_NORMAL);
    if (threads[cur_thread_no].h_thread == NULL) {
      for (cur_thread_no; cur_thread_no > 0; cur_thread_no--) {
        CloseHandle(threads[cur_thread_no - 1].h_thread);
        threads[cur_thread_no - 1].h_thread = NULL;
      }
      return TM_RETURN_VALUE_UNEXPECTED_ERROR;
    }
  }

  /* Start threads, but don't resume if in pause mode */
  for (cur_thread_no = 0; cur_thread_no < num_threads; cur_thread_no++) {
    if (!execution_paused) {
      ResumeThread(threads[cur_thread_no].h_thread);
    }
  }

  /* Wait for every thread to end */
  WaitForAllThreadsToTerminate();

  /* Everything ok */
  if (execution_cancelled) {
    return TM_RETURN_VALUE_EXECUTION_CANCELLED;
  } else {
    return TM_RETURN_VALUE_OK;
  }
}

/* Thread For Loop */
DWORD ThreadManagerClass::ThreadForLoop(LPVOID lp_parameter) {
  /* Locals */
  ForLoopStruct* for_loop_parameters = (ForLoopStruct*)lp_parameter;
  int64_t index;

  switch (for_loop_parameters->schedule_type) {
    case TM_SCHEDULE_STATIC:
      /* Loop through iterations */
      for (index = for_loop_parameters->initial_value;
           (for_loop_parameters->increment < 0)
               ? index >= for_loop_parameters->final_value
               : index <= for_loop_parameters->final_value;
           index += for_loop_parameters->increment) {
        /* Check if this is the last iteration */
        if (index == for_loop_parameters->final_value) {
          for_loop_parameters->thread_manager->any_thread_on_last_iteration =
              true;
        }
        /* Call the user function */
        switch (for_loop_parameters->thread_proc(
            for_loop_parameters->p_parameter, index)) {
          case TM_RETURN_VALUE_OK:
            break;
          case TM_RETURN_VALUE_TERMINATE_ALL_THREADS:
            for_loop_parameters->thread_manager->terminate_all_threads = true;
            break;
          default:
            break;
        }
        /* Check if the execution was cancelled */
        if (for_loop_parameters->thread_manager->terminate_all_threads) {
          break;
        }
      }
      break;
    case TM_SCHEDULE_DYNAMIC:
      return TM_RETURN_VALUE_INVALID_PARAM;
      break;
    case TM_SCHEDULE_GUIDED:
      return TM_RETURN_VALUE_INVALID_PARAM;
      break;
    case TM_SCHEDULE_RUNTIME:
      return TM_RETURN_VALUE_INVALID_PARAM;
      break;
  }

  return TM_RETURN_VALUE_OK;
}

} /* namespace muehle */
