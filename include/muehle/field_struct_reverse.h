#ifndef MUEHLE_FIELD_STRUCT_REVERSE_H_
#define MUEHLE_FIELD_STRUCT_REVERSE_H_

#include <vector>

#include "muehle/field_struct_variables.h"

namespace muehle {

/* Functions related to get the predecessor states */
class FieldStructReverse : virtual public FieldStructVariables {
 public:
  void GetPredecessors(std::vector<FieldStructTypes::Core>& pred_fields) const;

 private:
  /* Get predecessors */
  void GetPredecessors_normalMove(
      std::vector<FieldStructTypes::Core>& pred_fields,
      FieldStructReverse& field, bool mill_was_closed) const;
  void GetPredecessors_jumpingPhase(
      std::vector<FieldStructTypes::Core>& pred_fields,
      FieldStructReverse& field, bool mill_was_closed) const;
  void GetPredecessors_settingPhase(
      std::vector<FieldStructTypes::Core>& pred_fields,
      FieldStructReverse& field, bool mill_was_closed) const;
  void GetPredecessors_stoneRemove(
      std::vector<FieldStructTypes::Core>& pred_fields,
      FieldStructReverse& field) const;

  bool StorePredecessor(std::vector<FieldStructTypes::Core>& pred_fields,
                        const FieldStructReverse& field) const;
  bool AnyLonelyStone(const FieldStructReverse& field,
                      FieldPos removed_from) const;
};
} /* namespace muehle */

#endif /* MUEHLE_FIELD_STRUCT_REVERSE_H_ */
