// -*- C++ -*-
#ifndef _opt_sched_explorer_h_
#define _opt_sched_explorer_h_

#include "opt/sched/common.h"

namespace iroha {
namespace opt {
namespace sched {

class Explorer {
public:
  Explorer(WirePlanSet *wps);

  void SetInitialAllocation();
  bool MaySetNextAllocationPlan();

private:
  // Avoids too large muxes for resources.
  bool MayResolveTooManyResourceUses();
  bool ExploreNewPlan();
  bool SetNewPlan(WirePlan *wp);
  int GetUsageRate(ResourceEntry *re, int num_replicas,
		   const map<ResourceEntry *, int> &usage);
  bool HadSufficientImprovement();

  WirePlanSet *wps_;
};

}  // namespace sched
}  // namespace opt
}  // namespace iroha

#endif  // _opt_sched_explorer_h_
