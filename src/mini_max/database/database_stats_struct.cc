#include "muehle/mini_max/database/database_stats_struct.h"

namespace muehle {
namespace mini_max {
namespace database {

bool DatabaseStatsStruct::operator==(const DatabaseStatsStruct& other) const {
  return completed == other.completed && num_layers == other.num_layers;
}

}  // namespace database
}  // namespace mini_max
} // namespace muehle
