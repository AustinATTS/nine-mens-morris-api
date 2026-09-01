#include "muehle/mini_max/alpha_beta/common_thread_vars.h"

namespace muehle {

// Implementation goes here.

bool mini_max::CommonThreadVars::LoadDataToBuffer() {
}

bool mini_max::CommonThreadVars::Flush() {
}

mini_max::CommonThreadVars::CommonThreadVars(CommonThreadVars const& master) {
}

mini_max::CommonThreadVars::CommonThreadVars(unsigned int layer_number,
    const std::wstring& file_path, int64_t target_file_size,
    int64_t& rough_total_num_states_processed,
    int64_t& total_num_states_processed, Logger& log) {
}

mini_max::CommonThreadVars::~CommonThreadVars() {
}

bool mini_max::CommonThreadVars::ReadByte(int64_t possition_in_file,
    unsigned char& data) {
}

bool mini_max::CommonThreadVars::WriteByte(int64_t possition_in_file,
    unsigned char data) {
}

void mini_max::CommonThreadVars::Reduce() {
  ThreadVarsArrayItem::Reduce();
}
}  // namespace muehle
