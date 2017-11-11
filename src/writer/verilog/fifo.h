// -*- C++ -*-
#ifndef _writer_verilog_fifo_h_
#define _writer_verilog_fifo_h_

#include "writer/verilog/common.h"
#include "writer/verilog/resource.h"

namespace iroha {
namespace writer {
namespace verilog {

// Fifo can have depth and will supersede Channel (WIP).
class Fifo : public Resource {
public:
  Fifo(const IResource &res, const Table &table);

  virtual void BuildResource() override;
  virtual void BuildInsn(IInsn *insn, State *st) override;

  static string RReq(const IResource &res, const IResource *accessor);
  static string RAck(const IResource &res, const IResource *accessor);
  static string RData(const IResource &res);
  static string RDataBuf(const IResource &reader);
  static string WReq(const IResource &res, const IResource *accessor);
  static string WAck(const IResource &res, const IResource *accessor);
  static string WData(const IResource &res, const IResource *accessor);

private:
  string WritePtr();
  string ReadPtr();
  string WritePtrBuf();
  string ReadPtrBuf();
  string Full();
  string Empty();
  static string PinPrefix(const IResource &res, const IResource *accessor);
  void BuildMemoryInstance();
  void BuildWires();
  void BuildHandShake();
  void BuildAccessConnectionsAll();
  void BuildController();
  void BuildReq(bool is_write, const vector<IResource *> &accessors);
  void BuildAck(bool is_write, const vector<IResource *> &accessors);
  void BuildAckAssigns(bool is_write, const string &ack,
		       const vector<IResource *> &accessors,
		       const string &indent,
		       ostream &os);
};

}  // namespace verilog
}  // namespace writer
}  // namespace iroha

#endif  // _writer_verilog_fifo_h_
