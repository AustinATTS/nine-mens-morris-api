#include "muehle/mini_max/retro_analysis/state_queue.h"

namespace muehle {

// Implementation goes here.

mini_max::retro_analysis::StateQueue::StateQueue(Logger& log,
    const std::wstring& file_directory, unsigned int thread_no) {
}

mini_max::retro_analysis::StateQueue::~StateQueue() {
}

mini_max::retro_analysis::StateQueue::StateQueue(StateQueue&& other) noexcept {
}

mini_max::retro_analysis::StateQueue& mini_max::retro_analysis::StateQueue::
operator=(StateQueue&& other) noexcept {
}

bool mini_max::retro_analysis::StateQueue::Resize(PlyInfoVarType ply_number,
    size_t number_of_knots) {
}

bool mini_max::retro_analysis::StateQueue::PushBack(
    const StateAddressStruct& state, PlyInfoVarType ply_number,
    StateNumberVarType number_of_knots) {
}

bool mini_max::retro_analysis::StateQueue::PopFront(StateAddressStruct& state,
    PlyInfoVarType ply_number) {
}

unsigned int mini_max::retro_analysis::StateQueue::Size(
    PlyInfoVarType ply_number) {
}
}  // namespace muehle
