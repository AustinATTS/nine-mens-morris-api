#ifndef MUEHLE_FIELD_STRUCT_H_
#define MUEHLE_FIELD_STRUCT_H_

#include <array>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "muehle/field_struct_forward.h"
#include "muehle/field_struct_reverse.h"

namespace muehle {

/* Class representing the field. This master class is supposed to be used by the
 * class consumers. */
class FieldStruct : public FieldStructForward, public FieldStructReverse {
  friend class StateAddressing;

 public:
  /* Constructor */
  FieldStruct();
  FieldStruct(const FieldStruct& other);
  ~FieldStruct();

  void GetPredecessors_2(
      std::vector<FieldStructTypes::Core>& pred_fields) const;

  /* Operators */
  bool operator==(const FieldStruct& other) const;
};
}  // namespace muehle

#endif  // MUEHLE_FIELD_STRUCT_H_
