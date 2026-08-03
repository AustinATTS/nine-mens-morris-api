#ifndef MUEHLE_MUEHLE_AI_H_
#define MUEHLE_MUEHLE_AI_H_
#include "muehle/field_struct.h"

namespace muehle {

/* Base class representing the AI */
class MuehleAI {
 public:
  /* Constructor / destructor */
  MuehleAI() = default;
  ~MuehleAI() = default;

  /* Functions */
  virtual void Play(const FieldStruct& the_field, MoveInfo& move) = 0;
};
}  // namespace muehle

#endif  // MUEHLE_MUEHLE_AI_H_
