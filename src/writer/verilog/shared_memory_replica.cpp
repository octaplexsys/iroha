#include "writer/verilog/shared_memory_replica.h"

#include "iroha/i_design.h"
#include "writer/module_template.h"
#include "writer/verilog/embed.h"
#include "writer/verilog/internal_sram.h"
#include "writer/verilog/module.h"
#include "writer/verilog/ports.h"
#include "writer/verilog/table.h"

namespace iroha {
namespace writer {
namespace verilog {

SharedMemoryReplica::SharedMemoryReplica(const IResource &res,
					 const Table &table)
  : ArrayResource(res, table) {
}

void SharedMemoryReplica::BuildResource() {
  IResource *parent = res_.GetParentResource();
  IArray *array = parent->GetArray();
  InternalSRAM *sram =
    tab_.GetEmbeddedModules()->RequestInternalSRAM(*tab_.GetModule(),
						   *array, 2);
  ostream &es = tmpl_->GetStream(kEmbeddedInstanceSection);
  string name = sram->GetModuleName();
  string inst = name + "_inst_" + Util::Itoa(tab_.GetITable()->GetId()) +
    "_" + Util::Itoa(res_.GetId());
  auto *ports = tab_.GetPorts();
  es << "  " << name << " " << inst << "("
     << ".clk(" << ports->GetClk() << ")"
     << ", ." << sram->GetResetPinName() << "(" << ports->GetReset() << ")"
     << ", ." << sram->GetAddrPin(0) << "(" << SigName("addr") << ")"
     << ", ." << sram->GetRdataPin(0) << "(" << SigName("rdata") << ")"
     << ", ." << sram->GetWenPin(0) << "(1'b0)"
     << ");\n";
  ostream &rs = tab_.ResourceSectionStream();
  rs << "  reg " << sram->AddressWidthSpec() << SigName("addr") << ";\n"
     << "  wire " << sram->DataWidthSpec() << SigName("rdata") << ";\n";
}

void SharedMemoryReplica::BuildInsn(IInsn *insn, State *st) {
  BuildMemInsn(insn, st);
}

void SharedMemoryReplica::CollectNames(Names *names) {
}

IArray *SharedMemoryReplica::GetArray() {
  IResource *parent = res_.GetParentResource();
  return parent->GetArray();
}

}  // namespace verilog
}  // namespace writer
}  // namespace iroha
