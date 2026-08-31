#ifndef MUEHLE_MINI_MAX_DATABASE_DATABASE_STATS_STRUCT_H_
#define MUEHLE_MINI_MAX_DATABASE_DATABASE_STATS_STRUCT_H_

namespace muehle {
namespace mini_max {
namespace database {

/* Basic information about the database */
struct DatabaseStatsStruct {
  bool completed = false;      /* True if all states have been calculated */
  unsigned int num_layers = 0; /* Number of layers */

  bool operator==(const DatabaseStatsStruct &other) const;
};

}  // namespace database
}  // namespace mini_max
}  // namespace muehle

#endif  // MUEHLE_MINI_MAX_DATABASE_DATABASE_STATS_STRUCT_H_
