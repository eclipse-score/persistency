#ifndef TEST_OBJECT_SNAPSHOT_HPP
#define TEST_OBJECT_SNAPSHOT_HPP

#include "../cit/test_snapshots.hpp"

Scenario::Ptr snapshot_count_count_scenario{new SnapshotCount{}};
Scenario::Ptr snapshot_count_max_count_scenario{new SnapshotMaxCount{}};
Scenario::Ptr snapshot_count_restore_scenario{new SnapshotRestore{}};
Scenario::Ptr snapshot_count_paths_scenario{new SnapshotPaths{}};

ScenarioGroup::Ptr snapshot_group{new ScenarioGroupImpl{
    "snapshots",
{snapshot_count_count_scenario, snapshot_count_max_count_scenario, snapshot_count_restore_scenario,
    snapshot_count_paths_scenario},{}}};

#endif //TEST_OBJECT_SNAPSHOT_HPP