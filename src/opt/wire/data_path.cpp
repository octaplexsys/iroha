#include "opt/wire/data_path.h"

#include "iroha/i_design.h"
#include "iroha/stl_util.h"
#include "opt/bb_set.h"
#include "opt/debug_annotation.h"
#include "opt/delay_info.h"
#include "opt/wire/path_node.h"
#include "opt/wire/virtual_resource.h"

namespace iroha {
namespace opt {
namespace wire {

BBDataPath::BBDataPath(BB *bb, VirtualResourceSet *vrset)
  : bb_(bb), vrset_(vrset) {
}

BBDataPath::~BBDataPath() {
  STLDeleteSecondElements(&nodes_);
  STLDeleteValues(&edges_);
}

void BBDataPath::Build() {
  map<IInsn *, PathNode *> insn_to_node;
  int st_index = 0;
  for (IState *st : bb_->states_) {
    for (IInsn *insn : st->insns_) {
      PathNode *n = new PathNode(this, st_index, insn, vrset_->GetFromInsn(insn));
      nodes_[n->GetId()] = n;
      insn_to_node[insn] = n;
      auto &m = resource_node_map_[insn->GetResource()];
      m[st_index] = n;
    }
    ++st_index;
  }
  // insn and the index in outputs_[].
  map<IRegister *, pair<IInsn *, int> > output_to_insn;
  for (IState *st : bb_->states_) {
    // State local.
    for (IInsn *insn : st->insns_) {
      int oindex = 0;
      for (IRegister *oreg : insn->outputs_) {
	if (oreg->IsStateLocal()) {
	  output_to_insn[oreg] = make_pair(insn, oindex);
	}
	++oindex;
      }
    }
    for (IInsn *insn : st->insns_) {
      // Process inputs (W->R).
      for (IRegister *ireg : insn->inputs_) {
	BuildEdgeForReg(insn_to_node, output_to_insn,
			PathEdgeType::WRITE_READ, insn, ireg);
      }
      // Process outputs (W->W).
      for (IRegister *oreg : insn->outputs_) {
	BuildEdgeForReg(insn_to_node, output_to_insn,
			PathEdgeType::WRITE_WRITE, insn, oreg);
      }
    }
    // Not state local.
    for (IInsn *insn : st->insns_) {
      int oindex = 0;
      for (IRegister *oreg : insn->outputs_) {
	if (!oreg->IsStateLocal()) {
	  output_to_insn[oreg] = make_pair(insn, oindex);
	}
	++oindex;
      }
    }
  }
}

void BBDataPath::BuildEdgeForReg(map<IInsn *, PathNode *> &insn_to_node,
				 map<IRegister *, pair<IInsn *, int> > &output_to_insn,
				 PathEdgeType type, IInsn *insn, IRegister *reg) {
  auto p = output_to_insn[reg];
  IInsn *src_insn = p.first;
  int oindex = p.second;
  if (src_insn != nullptr) {
    PathNode *src_node = insn_to_node[src_insn];
    PathNode *this_node = insn_to_node[insn];
    BuildEdge(type, src_node, oindex, this_node);
  }
}

void BBDataPath::BuildEdge(PathEdgeType type, PathNode *src_node, int oindex,
			   PathNode *this_node) {
  // Use current number of edge as the edge id.
  int edge_id = edges_.size() + 1;
  PathEdge *edge = new PathEdge(edge_id, type,
				src_node, this_node, oindex);
  edges_.insert(edge);
  this_node->source_edges_[edge->GetId()] = edge;
  src_node->sink_edges_[edge->GetId()] = edge;
}

void BBDataPath::SetDelay(DelayInfo *dinfo) {
  for (auto &p : nodes_) {
    PathNode *n = p.second;
    n->SetAccumlatedDelayFromLeaf(-1);
    n->SetNodeDelay(dinfo->GetInsnDelay(n->GetInsn()));
  }
  for (auto &p : nodes_) {
    PathNode *n = p.second;
    SetAccumlatedDelay(dinfo, n);
  }
}

void BBDataPath::SetAccumlatedDelay(DelayInfo *dinfo, PathNode *node) {
  if (node->GetAccumlatedDelayFromLeaf() >= 0) {
    // Already computed.
    return;
  }
  // Compute delayes of preceding nodes.
  for (auto &p : node->source_edges_) {
    PathEdge *edge = p.second;
    if (edge->IsWtoR()) {
      PathNode *source_node = edge->GetSourceNode();
      SetAccumlatedDelay(dinfo, source_node);
    }
  }
  // Get the max value.
  int max_source_delay = 0;
  for (auto &p : node->source_edges_) {
    PathEdge *edge = p.second;
    if (edge->IsWtoR()) {
      PathNode *source_node = edge->GetSourceNode();
      if (max_source_delay < source_node->GetAccumlatedDelayFromLeaf()) {
	max_source_delay = source_node->GetAccumlatedDelayFromLeaf();
      }
    }
  }
  node->SetAccumlatedDelayFromLeaf(max_source_delay + node->GetNodeDelay());
}

void BBDataPath::Dump(ostream &os) {
  os << "DataPath BB: " << bb_->bb_id_ << "\n";
  for (auto &p : nodes_) {
    p.second->Dump(os);
  }
}

BB *BBDataPath::GetBB() {
  return bb_;
}

map<int, PathNode *> &BBDataPath::GetNodes() {
  return nodes_;
}

map<int, PathNode *> &BBDataPath::GetResourceNodeMap(IResource *res) {
  return resource_node_map_[res];
}

DataPathSet::DataPathSet() {
}

DataPathSet::~DataPathSet() {
  STLDeleteSecondElements(&data_paths_);
}

void DataPathSet::Build(BBSet *bset) {
  vres_set_.reset(new VirtualResourceSet(bset->GetTable()));
  bbs_ = bset;
  for (BB *bb : bbs_->bbs_) {
    BBDataPath *dp = new BBDataPath(bb, vres_set_.get());
    data_paths_[bb->bb_id_] = dp;
    dp->Build();
  }
  vres_set_->BuildDefaultBinding();
}

void DataPathSet::SetDelay(DelayInfo *dinfo) {
  for (auto &p : data_paths_) {
    BBDataPath *dp = p.second;
    dp->SetDelay(dinfo);
  }
}

void DataPathSet::Dump(DebugAnnotation *an) {
  ostream &os = an->GetDumpStream();
  os << "DataPathSet table: " << bbs_->GetTable()->GetId() << "\n";
  for (auto &p : data_paths_) {
    p.second->Dump(os);
  }
}

map<int, BBDataPath *> &DataPathSet::GetPaths() {
  return data_paths_;
}

BBSet *DataPathSet::GetBBSet() {
  return bbs_;
}

VirtualResourceSet *DataPathSet::GetVirtualResourceSet() {
  return vres_set_.get();
}

}  // namespace wire
}  // namespace opt
}  // namespace iroha
