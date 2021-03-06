// -*- C++ -*-
#ifndef _writer_verilog_ext_task_call_h_
#define _writer_verilog_ext_task_call_h_

#include "writer/verilog/resource.h"

namespace iroha {
namespace writer {
namespace verilog {

class ExtTaskCall : public Resource {
public:
  ExtTaskCall(const IResource &res, const Table &table);
  virtual ~ExtTaskCall() {};

  virtual void BuildResource();
  virtual void BuildInsn(IInsn *insn, State *st);

private:
  void BuildExtTaskCallResource();
  void BuildTaskCallInsn(IInsn *insn, State *st);
  void BuildTaskWaitInsn(IInsn *insn, State *st);
  void BuildFlowCallInsn(IInsn *insn, State *st);
  void BuildFlowResultInsn(IInsn *insn, State *st);
  string ResCaptureReg(int nth);
  bool IsEmbedded() const;
  bool UseHandShake() const;
  // Finds corresponding ext-task-wait resource.
  const IResource *GetWaitResource() const;
  void AddPort(const string &name, const string &wire_name,
	       bool is_output, int width,
	       string *connection);
  void AddIO(string *connection);
  void AddIOPorts(bool is_output, string *connection);
};

}  // namespace verilog
}  // namespace writer
}  // namespace iroha

#endif  // _writer_verilog_ext_task_call_h_
