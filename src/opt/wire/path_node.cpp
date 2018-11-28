#include "opt/wire/path_node.h"

#include "iroha/i_design.h"
#include "iroha/resource_class.h"

namespace iroha {
namespace opt {
namespace wire {

PathEdge::PathEdge(int id, PathNode *source_node, PathNode *sink_node,
		   int source_reg_index)
  : id_(id), source_node_(source_node), source_reg_index_(source_reg_index),
    sink_node_(sink_node) {
}

int PathEdge::GetId() {
  return id_;
}

IRegister *PathEdge::GetSourceReg() {
  return source_node_->GetInsn()->outputs_[source_reg_index_];
}

void PathEdge::SetSourceReg(IRegister *reg) {
  source_node_->GetInsn()->outputs_[source_reg_index_] = reg;
}

PathNode *PathEdge::GetSourceNode() {
  return source_node_;
}

int PathEdge::GetSourceRegIndex() {
  return source_reg_index_;
}

PathNode *PathEdge::GetSinkNode() {
  return sink_node_;
}

PathNode::PathNode(BBDataPath *path, int st_index, IInsn *insn, VirtualResource *vres)
  : state_local_delay_(0), path_(path),
    initial_st_index_(st_index), final_st_index_(st_index),
    insn_(insn), vres_(vres), node_delay_(0), accumlated_delay_from_leaf_(0) {
}

int PathNode::GetId() {
  return insn_->GetId();
}

int PathNode::GetInitialStIndex() {
  return initial_st_index_;
}

int PathNode::GetFinalStIndex() {
  return final_st_index_;
}

void PathNode::SetFinalStIndex(int final_st_index) {
  final_st_index_ = final_st_index;
}

VirtualResource *PathNode::GetVirtualResource() {
  return vres_;
}

int PathNode::GetNodeDelay() {
  return node_delay_;
}

void PathNode::SetNodeDelay(int node_delay) {
  node_delay_ = node_delay;
}

int PathNode::GetAccumlatedDelayFromLeaf() {
  return accumlated_delay_from_leaf_;
}

void PathNode::SetAccumlatedDelayFromLeaf(int accumlated_delay_from_leaf) {
  accumlated_delay_from_leaf_ = accumlated_delay_from_leaf;
}

void PathNode::Dump(ostream &os) {
  os << "Node: " << GetId() << "@" << initial_st_index_ << " "
     << node_delay_ << " " << accumlated_delay_from_leaf_ << "\n";
  if (source_edges_.size() > 0) {
    for (auto &p : source_edges_) {
      int source_node_id = p.first;
      os << " " << source_node_id;
    }
    os << "\n";
  }
}

IInsn *PathNode::GetInsn() {
  return insn_;
}

bool PathNode::IsTransition() {
  resource::IsTransition(*(insn_->GetResource()->GetClass()));
}

}  // namespace wire
}  // namespace opt
}  // namespace iroha
