#include "writer/verilog/axi/axi_port.h"

#include "iroha/i_design.h"
#include "iroha/resource_params.h"
#include "writer/verilog/axi/axi_controller.h"
#include "writer/verilog/module.h"
#include "writer/verilog/port.h"
#include "writer/verilog/port_set.h"
#include "writer/verilog/table.h"
#include "writer/verilog/shared_memory.h"
#include "writer/verilog/shared_memory_accessor.h"
#include "writer/verilog/wire/wire_set.h"

namespace iroha {
namespace writer {
namespace verilog {
namespace axi {

AxiPort::AxiPort(const IResource &res, const Table &table)
  : Resource(res, table) {
  reset_polarity_ = tab_.GetModule()->GetResetPolarity();
}

void AxiPort::OutputSRAMConnection(ostream &os) {
  const string &clk = tab_.GetPortSet()->GetClk();
  const string &rst = tab_.GetPortSet()->GetReset();
  const IResource *mem = res_.GetParentResource();
  string rn = SharedMemory::GetName(*mem);
  int idx = 0;
  int excl = 0;
  const IResource *accessor;
  string addr, wdata, rdata, wen;
  if (IsExclusiveAccessor()) {
    idx = 1;
    excl = 1;
    accessor = nullptr;
    // Directly connected to port 1 (of 0 and 1).
    addr = SharedMemory::MemoryAddrPin(*mem, idx, accessor);
    wdata = SharedMemory::MemoryWdataPin(*mem, idx, accessor);
    rdata = SharedMemory::MemoryRdataPin(*mem, idx);
    wen = SharedMemory::MemoryWenPin(*mem, idx, accessor);
  } else {
    accessor = &res_;
    // Connection via WireSet.
    addr = SharedMemoryAccessor::AddrSrc(res_);
    wdata = SharedMemoryAccessor::WDataSrc(res_);
    wen = SharedMemoryAccessor::WEnSrc(res_);
    rdata = wire::Names::AccessorWire(rn, &res_, "rdata");
  }
  os << ".clk("<< clk << "), "
     << "." << AxiController::ResetName(reset_polarity_)
     << "(" << rst << "), "
     << ".sram_addr(" << addr << "), "
     << ".sram_wdata(" << wdata << "), "
     << ".sram_rdata(" << rdata << "), "
     << ".sram_wen(" << wen << "), "
     << ".sram_EXCLUSIVE(1'b" << excl << ")";
  if (excl) {
    os << ", .sram_req(/*not connected*/), .sram_ack(1'b1)";
  } else {
    os << ", .sram_req(" << wire::Names::AccessorWire(rn, &res_, "req")
       << "), .sram_ack(" << wire::Names::AccessorWire(rn, &res_, "ack")
       << ")";
  }
}

PortConfig AxiPort::GetPortConfig(const IResource &res) {
  PortConfig cfg;
  cfg.prefix = res.GetParams()->GetPortNamePrefix();
  cfg.axi_addr_width = res.GetParams()->GetAddrWidth();
  const IResource *mem_res = res.GetParentResource();
  IArray *array = mem_res->GetArray();
  cfg.sram_addr_width = array->GetAddressWidth();
  cfg.data_width = array->GetDataType().GetWidth();
  return cfg;
}

string AxiPort::PortSuffix() {
  const ITable *tab = res_.GetTable();
  return Util::Itoa(tab->GetId()) + "_" + Util::Itoa(res_.GetId());
}

string AxiPort::AddrPort() {
  return "axi_addr" + PortSuffix();
}

string AxiPort::WenPort() {
  return "axi_wen" + PortSuffix();
}

string AxiPort::ReqPort() {
  return "axi_req" + PortSuffix();
}

string AxiPort::AckPort() {
  return "axi_ack" + PortSuffix();
}

bool AxiPort::IsExclusiveAccessor() {
  auto *params = res_.GetParams();
  if (params->GetSramPortIndex() == "0") {
    return false;
  }
  return true;
}

}  // namespace axi
}  // namespace verilog
}  // namespace writer
}  // namespace iroha
