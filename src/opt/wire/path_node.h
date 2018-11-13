// -*- C++ -*-
#ifndef _opt_wire_path_node_h_
#define _opt_wire_path_node_h_

#include "opt/wire/common.h"

namespace iroha {
namespace opt {
namespace wire {

// Data path (BBDataPath) is a graph comprised of nodes and edges.
//
// ------------                    ------
// |Node(Insn)|-->Edge(Register)-->|Node|
// ------------                    ------

// Represents an IRegister flows from its source insn to sink.
class PathEdge {
public:
  PathEdge(int id, PathNode *source_node, PathNode *sink_node,
	   int source_reg_index);

  int GetId();
  IRegister *GetSourceReg();
  void SetSourceReg(IRegister *reg);
  int GetSourceRegIndex();
  PathNode *GetSourceNode();
  PathNode *GetSinkNode();

private:
  int id_;
  PathNode *source_node_;
  int source_reg_index_;
  PathNode *sink_node_;
};

// Represents an IInsn and connected to other insns via PathEdge-s.
class PathNode {
public:
  PathNode(BBDataPath *path, int st_index, IInsn *insn);

  int GetId();
  void Dump(ostream &os);
  IInsn *GetInsn();
  int GetFinalStIndex();
  void SetFinalStIndex(int final_st_index);

  // Latency of this node (=insn).
  int node_delay_;
  // Scratch variable for Scheduler.
  int state_local_delay_;
  // Accumlated delay in the current state.
  int accumlated_delay_;
  // key-ed by source node id.
  map<int, PathEdge *> source_edges_;
  // key-ed by node id.
  map<int, PathEdge *> sink_edges_;

private:
  BBDataPath *path_;
  int initial_st_index_;
  int final_st_index_;
  IInsn *insn_;
};

}  // namespace wire
}  // namespace opt
}  // namespace iroha

#endif  // _opt_wire_path_node_h_
