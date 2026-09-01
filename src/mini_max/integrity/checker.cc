#include "muehle/mini_max/integrity/checker.h"

namespace muehle {

// Implementation goes here.

mini_max::integrity::Checker::Checker(Logger& log, ThreadManagerClass& tm,
    database::Database& db, GameInterface& game) {
}

bool mini_max::integrity::Checker::TestSetSituationAndGetStateNum(
    unsigned int layer_number) {
}

bool mini_max::integrity::Checker::TestMoveAndUndo(unsigned int layer_number) {
}

bool mini_max::integrity::Checker::TestGetPredecessors(
    unsigned int layer_number) {
}

bool mini_max::integrity::Checker::TestGetPossibilities(
    unsigned int layer_number) {
}

bool mini_max::integrity::Checker::TestState(unsigned int layer_number,
    unsigned int state_number) {
}

bool mini_max::integrity::Checker::TestLayer(unsigned int layer_number) {
}

bool mini_max::integrity::Checker::TestIfSymStatesHaveSameValue(
    unsigned int layer_number) {
}

bool mini_max::integrity::Checker::StartTestThreads(unsigned int layer_number,
    DWORD thread_prc(void* p_parameter, int64_t index)) {
}

unsigned int mini_max::integrity::Checker::GetIncrement(
    unsigned int layer_number) {
}

DWORD mini_max::integrity::Checker::TestLayerThreadProc(void* p_parameter,
    int64_t index) {
}

DWORD mini_max::integrity::Checker::TestMoveAndUndoThreadProc(void* p_parameter,
    int64_t index) {
}

DWORD mini_max::integrity::Checker::TestSetSituationThreadProc(
    void* p_parameter, int64_t index) {
}

DWORD mini_max::integrity::Checker::TestGetPredecessorsThreadProc(
    void* p_parameter, int64_t index) {
}

DWORD mini_max::integrity::Checker::TestGetPossibilitiesThreadProc(
    void* p_parameter, int64_t index) {
}

DWORD mini_max::integrity::Checker::TestSymStatesSameValueThreadProc(
    void* p_parameter, int64_t index) {
}
}  // namespace muehle
