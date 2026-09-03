#ifndef MUEHLE_MINI_MAX_DATABASE_DATABASE_STATS_STRUCT_H_
#define MUEHLE_MINI_MAX_DATABASE_DATABASE_STATS_STRUCT_H_

namespace muehle {
namespace mini_max {
namespace database {

/* Basic information about the database */
struct DatabaseStatsStruct {
  /* True if all states have been calculated */
  bool completed = false;
  /* Number of layers */
  unsigned int num_layers = 0;

  bool operator==(const DatabaseStatsStruct &other) const;
};

} /* namespace database */
} /* namespace mini_max */
} /* namespace muehle */

#endif /* MUEHLE_MINI_MAX_DATABASE_DATABASE_STATS_STRUCT_H_ */
