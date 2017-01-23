// -*- C++ -*-
#ifndef _writer_verilog_internal_sram_h_
#define _writer_verilog_internal_sram_h_

#include "writer/verilog/common.h"

namespace iroha {
namespace writer {
namespace verilog {

class InternalSRAM {
public:
  InternalSRAM(const Module &mod, const IResource &res, int num_ports);

  void Write(ostream &os);

  string GetModuleName() const;
  string GetResetPinName() const;
  const IResource &GetResource() const;

  string GetAddrPin(int port) const;
  string GetRdataPin(int port) const;
  string GetWenPin(int port) const;
  string GetWdataPin(int port) const;

  string AddressWidthSpec() const;
  string DataWidthSpec() const;

private:
  static string WidthSpec(int w);
  void WriteInternal(ostream &os);
  string MaybePortPrefix(int port) const;

  const Module &mod_;
  const IResource &res_;
  int num_ports_;
  bool reset_polarity_;
};

}  // namespace verilog
}  // namespace writer
}  // namespace iroha

#endif  // _writer_verilog_internal_sram_h_
