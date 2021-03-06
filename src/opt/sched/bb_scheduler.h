// -*- C++ -*-
#ifndef _opt_sched_bb_scheduler_h_
#define _opt_sched_bb_scheduler_h_

#include "opt/sched/common.h"

namespace iroha {
namespace opt {
namespace sched {

// Schedules a BB.
class BBScheduler {
public:
  BBScheduler(BBDataPath *data_path, DelayInfo *delay_info,
	      ResourceConflictTracker *conflict_tracker);
  ~BBScheduler();

  void Schedule();

private:
  bool ScheduleNode(PathNode *n);
  void ClearSchedule();
  void ScheduleExclusive(PathNode *n, int min_st_index);
  void ScheduleNonExclusive(PathNode *n, int st_index);
  bool IsSchedulable();
  int GetMinStIndex(PathNode *n);
  int GetLocalDelayBeforeNode(PathNode *n, int st_index);
  int GetMinStByEdgeDependency(PathNode *n);
  bool CheckPrecedingNodesOfSameResource(PathNode *n);
  int GetMinStByPrecedingNodeOfSameResource(PathNode *n);

  BBDataPath *data_path_;
  DelayInfo *delay_info_;
  // latency to nodes.
  map<int, vector<PathNode *> > sorted_nodes_;
  ResourceConflictTracker *conflict_tracker_;
  std::unique_ptr<BBResourceTracker> resource_tracker_;
};

}  // namespace sched
}  // namespace opt
}  // namespace iroha

#endif  // _opt_sched_bb_scheduler_h_
