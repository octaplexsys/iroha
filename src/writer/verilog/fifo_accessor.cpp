#include "writer/verilog/fifo_accessor.h"

#include "design/design_util.h"
#include "iroha/insn_operands.h"
#include "iroha/i_design.h"
#include "iroha/resource_class.h"
#include "iroha/resource_params.h"
#include "writer/module_template.h"
#include "writer/verilog/fifo.h"
#include "writer/verilog/insn_writer.h"
#include "writer/verilog/module.h"
#include "writer/verilog/state.h"
#include "writer/verilog/table.h"

namespace iroha {
namespace writer {
namespace verilog {

FifoAccessor::FifoAccessor(const IResource &res, const Table &table)
  : Resource(res, table) {
}

void FifoAccessor::BuildResource() {
  auto *klass = res_.GetClass();
  if (resource::IsFifoReader(*klass)) {
    BuildReader();
  }
  if (resource::IsFifoWriter(*klass)) {
    BuildWriter();
  }
}

void FifoAccessor::BuildInsn(IInsn *insn, State *st) {
  IResource *res = insn->GetResource();
  auto *klass = res_.GetClass();
  if (resource::IsFifoReader(*klass)) {
    BuildReadInsn(insn, st);
  }
  if (resource::IsFifoWriter(*klass)) {
    BuildWriteInsn(insn, st);
  }
}

void FifoAccessor::BuildReader() {
  BuildReq(false);
  int dw = res_.GetParentResource()->GetParams()->GetWidth();
  ostream &rs = tab_.ResourceSectionStream();
  rs << "  reg " << Table::WidthSpec(dw)
     << Fifo::RDataBuf(res_) << ";\n";
}

void FifoAccessor::BuildWriter() {
  BuildReq(true);
}

void FifoAccessor::BuildReq(bool is_writer) {
  ostream &ss = tab_.StateOutputSectionStream();
  map<IState *, IInsn *> callers;
  CollectResourceCallers("", &callers);
  map<IState *, IInsn *> nw_callers;
  CollectResourceCallers(operand::kNoWait, &nw_callers);
  string sig;
  string ack;
  if (is_writer) {
    sig = Fifo::WReq(*(res_.GetParentResource()), &res_);
    ack = Fifo::WAck(*(res_.GetParentResource()), &res_);
  } else {
    sig = Fifo::RReq(*(res_.GetParentResource()), &res_);
    ack = Fifo::RAck(*(res_.GetParentResource()), &res_);
  }
  string req = JoinStatesWithSubState(callers, 0);
  if (req.empty()) {
    req = "0";
  }
  ss << "      " << sig << " <= (" << req << ") && !" << ack << ";\n";
  string nw_req = JoinStates(nw_callers);
  if (!nw_req.empty()) {
    ss << "      " << Fifo::WNoWait(*(res_.GetParentResource()), &res_)
       << " <= " << nw_req << ";\n";
  }
}

void FifoAccessor::BuildReadInsn(IInsn *insn, State *st) {
  ostream &os = st->StateBodySectionStream();
  static const char I[] = "          ";
  string st_name = InsnWriter::MultiCycleStateName(*(insn->GetResource()));
  os << I << "if (" << st_name << " == 0) begin\n"
     << I << "  if (" << Fifo::RAck(*(res_.GetParentResource()), &res_) << ") begin\n"
     << I << "    " << Fifo::RDataBuf(res_) << " <= " << Fifo::RData(*(res_.GetParentResource())) << ";\n"
     << I << "    " << st_name << " <= 3;\n"
     << I << "  end\n";
  os << I << "end\n";
  ostream &ws = tmpl_->GetStream(kInsnWireValueSection);
      ws << "  assign " << InsnWriter::InsnOutputWireName(*insn, 0)
	 << " = " << Fifo::RDataBuf(res_) << ";\n";
}

void FifoAccessor::BuildWriteInsn(IInsn *insn, State *st) {
  if (insn->GetOperand() == operand::kNoWait) {
    BuildNoWaitWriteInsn(insn, st);
    return;
  }
  ostream &os = st->StateBodySectionStream();
  static const char I[] = "          ";
  string st_name = InsnWriter::MultiCycleStateName(*(insn->GetResource()));
  os << I << "if (" << st_name << " == 0) begin\n"
     << I << "  " << Fifo::WData(*(res_.GetParentResource()), &res_) << " <= "
     << InsnWriter::RegisterValue(*insn->inputs_[0], tab_.GetNames()) << ";\n"
     << I << "  if (" << Fifo::WAck(*(res_.GetParentResource()), &res_)
     << ") begin\n"
     << I << "    " << st_name << " <= 3;\n"
     << I << "  end\n";
  os << I << "end\n";
}

void FifoAccessor::BuildNoWaitWriteInsn(IInsn *insn, State *st) {
  ostream &os = st->StateBodySectionStream();
  static const char I[] = "          ";
  os << I << Fifo::WData(*(res_.GetParentResource()), &res_) << " <= "
     << InsnWriter::RegisterValue(*insn->inputs_[0], tab_.GetNames()) << ";\n";
}

bool FifoAccessor::UseNoWait(const IResource *accessor) {
  vector<IInsn *> insns = DesignUtil::GetInsnsByResource(accessor);
  for (auto *insn : insns) {
    if (insn->GetOperand() == operand::kNoWait) {
      return true;
    }
  }
  return false;
}

}  // namespace verilog
}  // namespace writer
}  // namespace iroha
