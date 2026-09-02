#include "muehle/mini_max/retro_analysis/state_queue.h"

#include "muehle/mini_max/return_values.h"

namespace muehle {

/* Constructor */
mini_max::retro_analysis::StateQueue::StateQueue(
    Logger& log, const std::wstring& file_directory, unsigned int thread_no)
    : log(log),
      num_states_to_process(0),
      max_ply_info_value(0),
      file_directory(file_directory),
      thread_no(thread_no) {
  states_to_process.resize(PLYINFO_EXP_VALUE, nullptr);
}

/* Destructor */
mini_max::retro_analysis::StateQueue::~StateQueue() {
  for (unsigned int ply_counter = 0; ply_counter < states_to_process.size();
       ply_counter++) {
    SAFE_DELETE(states_to_process[ply_counter]);
  }
}

/* Move constructor */
mini_max::retro_analysis::StateQueue::StateQueue(StateQueue&& other) noexcept
    : log(log),
      states_to_process(std::move(other.states_to_process)),
      file_directory(std::move(other.file_directory)),
      num_states_to_process(other.num_states_to_process),
      max_ply_info_value(other.max_ply_info_value),
      thread_no(other.thread_no) {
  other.num_states_to_process = 0;
  other.max_ply_info_value = 0;
  other.thread_no = 0xffff;
}

/* Move assignment operator */
mini_max::retro_analysis::StateQueue&
mini_max::retro_analysis::StateQueue::operator=(StateQueue&& other) noexcept {
  if (this != &other) {
    log = std::move(other.log);
    states_to_process = std::move(other.states_to_process);
    file_directory = std::move(other.file_directory);
    num_states_to_process = other.num_states_to_process;
    max_ply_info_value = other.max_ply_info_value;
    thread_no = other.thread_no;
    other.num_states_to_process = 0;
    other.max_ply_info_value = 0;
    other.thread_no = 0xffff;
  }
  return *this;
}

/* Resizes the state queue to the given number of knots */
bool mini_max::retro_analysis::StateQueue::Resize(PlyInfoVarType ply_number,
                                                  size_t number_of_knots) {
  /* Checks */
  if (ply_number >= states_to_process.size()) {
    log << "ERROR: Ply number must be smaller than the size of "
           "states_to_process! ply_number:"
        << ply_number << ", max allowed: " << (states_to_process.size() - 1)
        << "\n";
    return ReturnValues::FalseOrStop();
  }
  if (number_of_knots <= 0) {
    log << "ERROR: Number of knots must be greater than 0! ply_number:"
        << ply_number << "\n";
    return ReturnValues::FalseOrStop();
  }

  /* Locals */
  std::wstringstream ss_states_to_process_file_path;
  std::wstringstream ss_states_to_process_path;

  /* Create file name */
  ss_states_to_process_path << file_directory
                            << (file_directory.size() ? "/" : "")
                            << "states_to_process";
  CreateDirectory(ss_states_to_process_path.str().c_str(), NULL);
  ss_states_to_process_file_path.str(L"");
  ss_states_to_process_file_path
      << ss_states_to_process_path.str()
      << "/states_to_process_with_ply_counter=" << ply_number
      << "and_thread=" << thread_no << ".dat";

  /* Create cyclic array for this ply number */
  states_to_process[ply_number] =
      new CyclicArray(BLOCK_SIZE_IN_CYCLIC_ARRAY * sizeof(StateAddressStruct),
                      (number_of_knots / BLOCK_SIZE_IN_CYCLIC_ARRAY) + 1,
                      ss_states_to_process_file_path.str().c_str(), log);

  log.Log(Logger::LogLevel::trace,
          L"Created cyclic array: " + ss_states_to_process_file_path.str());
  return true;
}

/* Adds a state to the queue for the given ply number. Therefore a cyclic array
 * is used. The cyclic array is stored in a file. */
bool mini_max::retro_analysis::StateQueue::PushBack(
    const StateAddressStruct& state, PlyInfoVarType ply_number,
    StateNumberVarType number_of_knots) {
  /* Checks if the ply number is valid */
  if (ply_number < 0) {
    log << "ERROR: Ply number must be greater or equal to 0! ply_number:"
        << ply_number << "\n";
    return ReturnValues::FalseOrStop();
  }
  if (number_of_knots <= 0) {
    log << "ERROR: Number of knots must be greater than 0! ply_number:"
        << ply_number << "\n";
    return ReturnValues::FalseOrStop();
  }
  if (state.state_number >= number_of_knots) {
    log << "ERROR: State number must be smaller than the number of knots! "
           "state_number:"
        << state.state_number << " number_of_knots:" << number_of_knots << "\n";
    return ReturnValues::FalseOrStop();
  }

  /* Resize vector if too small */
  if (ply_number >= states_to_process.size()) {
    states_to_process.resize(
        std::max((size_t)(ply_number + 1), 10 * states_to_process.size()),
        nullptr);
    log.Log(Logger::LogLevel::warning,
            L"states_to_process resized to " +
                std::to_wstring(states_to_process.size()));
  }

  /* Set max ply info value */
  if (ply_number > max_ply_info_value) {
    max_ply_info_value = ply_number;
  }

  /* Initialise cyclic array if necessary */
  if (states_to_process[ply_number] == nullptr) {
    return log.Log(Logger::LogLevel::error,
                   L"ERROR: states_to_process[" + std::to_wstring(ply_number) +
                       L"] not initialised! Call Resize() first"),
           ReturnValues::FalseOrStop();
  }

  /* Add state */
  if (!states_to_process[ply_number]->AddBytes(sizeof(StateAddressStruct),
                                               (unsigned char*)&state)) {
    log << "ERROR: Cyclic list too small! num_states_to_process:"
        << num_states_to_process << "\n";
    log << "ERROR: GetNumBlocks() = "
        << states_to_process[ply_number]->GetNumBlocks() << "\n";
    return ReturnValues::FalseOrStop();
  }

  /* Everything was fine */
  num_states_to_process++;
  return true;
}

/* Sets the parameter 'state' to the first state in the queue for the given ply
 * number.
 * Returns false if the queue is empty. */
bool mini_max::retro_analysis::StateQueue::PopFront(StateAddressStruct& state,
                                                    PlyInfoVarType ply_number) {
  /* Check parameter */
  if (ply_number >= states_to_process.size() ||
      states_to_process[ply_number] == nullptr) {
    return false;
  }
  if (num_states_to_process == 0) {
    return false;
  }
  if (states_to_process[ply_number]->BytesAvailable() <
      sizeof(StateAddressStruct)) {
    return false;
  }

  /* Take state */
  if (!states_to_process[ply_number]->TakeBytes(sizeof(StateAddressStruct),
                                                (unsigned char*)&state)) {
    log << "ERROR: TakeBytes failed! num_states_to_process:"
        << num_states_to_process << "\n";
    return ReturnValues::FalseOrStop();
  }

  /* Everything was fine */
  num_states_to_process--;
  return true;
}

/* Returns the number of states in the queue for the given ply number */
unsigned int mini_max::retro_analysis::StateQueue::Size(
    PlyInfoVarType ply_number) {
  /* Check parameter */
  if (ply_number >= states_to_process.size() ||
      states_to_process[ply_number] == nullptr) {
    return 0;
  }

  /* Get size */
  return states_to_process[ply_number]->BytesAvailable() /
         sizeof(StateAddressStruct);
}

}  // namespace muehle
