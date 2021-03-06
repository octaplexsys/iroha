// A shared register can have shared register writers, readers and
// dataflow entries (equivalent to readers).
//
// shared-reg-writer generates connections from shared-reg-writer to shared-reg.
// shared-reg generates connections from shared-reg to shared-reg-reader.

#include "writer/verilog/shared_reg.h"

#include "design/design_util.h"
#include "iroha/i_design.h"
#include "iroha/logging.h"
#include "iroha/resource_class.h"
#include "iroha/resource_params.h"
#include "writer/connection.h"
#include "writer/module_template.h"
#include "writer/verilog/insn_writer.h"
#include "writer/verilog/module.h"
#include "writer/verilog/port.h"
#include "writer/verilog/shared_reg_accessor.h"
#include "writer/verilog/shared_reg_ext_writer.h"
#include "writer/verilog/state.h"
#include "writer/verilog/table.h"
#include "writer/verilog/wire/accessor_info.h"
#include "writer/verilog/wire/wire_set.h"

namespace iroha {
namespace writer {
namespace verilog {

SharedReg::SharedReg(const IResource &res, const Table &table)
  : Resource(res, table), has_default_value_(false),
    default_value_(0),
    need_write_arbitration_(false) {
  auto *klass = res_.GetClass();
  CHECK(resource::IsSharedReg(*klass));
  auto *params = res_.GetParams();
  string unused;
  params->GetExtOutputPort(&unused, &width_);
  has_default_value_ =
    params->GetDefaultValue(&default_value_);
  auto &readers =
    table.GetModule()->GetConnection().GetSharedRegReaders(&res_);
  for (auto *r : readers) {
    readers_.push_back(r);
  }
  auto &children =
    table.GetModule()->GetConnection().GetSharedRegChildren(&res_);
  for (auto *r : children) {
    readers_.push_back(r);
  }
  writers_ = table.GetModule()->GetConnection().GetSharedRegWriters(&res_);
  auto &ext_writers =
    table.GetModule()->GetConnection().GetSharedRegExtWriters(&res_);
  for (auto *w: ext_writers) {
    writers_.push_back(w);
  }
  if (writers_.size() > 0 || has_default_value_) {
    need_write_arbitration_ = true;
  }
  GetOptions(&use_notify_, &use_mailbox_);
}

void SharedReg::BuildResource() {
  ostream &rs = tab_.ResourceSectionStream();
  ostream &is = tab_.InitialValueSectionStream();
  rs << "  // shared-reg";
  if (use_notify_) {
    rs << " use-notify";
  }
  if (use_mailbox_) {
    rs << " use-mailbox";
  }
  rs << "\n";
  tab_.AddReg(RegName(res_), width_);
  // Reset value
  is << "      " << RegName(res_) << " <= ";
  int initial_value;
  if (res_.GetParams()->GetInitialValue(&initial_value)) {
    is << initial_value;
  } else if (has_default_value_) {
    is << default_value_;
  } else {
    is << 0;
  }
  is << ";\n";
  if (readers_.size() > 0) {
    ostream &rvs = tab_.ResourceValueSectionStream();
    string rrn = GetNameRW(res_, false);
    rvs << "  assign " << wire::Names::ResourceWire(rrn, "r")
	<< " = " << RegName(res_) << ";\n";
  }
  if (use_notify_) {
    BuildNotifier();
  }
  if (use_mailbox_) {
    BuildMailbox();
  }
  if (need_write_arbitration_) {
    // Priorities are
    // (1) Writers in this table
    // (2) Writers in other table
    // (3) Default output value
    ostream &os = tab_.StateOutputSectionStream();
    os << "      " << RegName(res_) << " <= ";
    string value;
    if (has_default_value_) {
      value = Util::Itoa(default_value_);
    } else {
      value = RegName(res_);
    }
    string rn = GetNameRW(res_, true);
    string en = wire::Names::ResourceWire(rn, "wen");
    if (use_notify_) {
      en += " | " + wire::Names::ResourceWire(rn, "notify");
    }
    if (use_mailbox_) {
      en += " | (" + wire::Names::ResourceWire(rn, "put_req") + " && " +
	wire::Names::ResourceWire(rn, "put_ack") + ")";
    }
    value = "(" + en + ") ? " + wire::Names::ResourceWire(rn, "w")
      + " : (" + value + ")";
    os << SelectValueByState(value);
    os << ";\n";
  }
  BuildAccessorWireW();
  BuildAccessorWireR();
}

void SharedReg::BuildMailbox() {
  tab_.AddReg(RegMailboxName(res_), 0);
  tab_.AddReg(RegMailboxPutAckName(res_), 0);
  tab_.AddReg(RegMailboxGetAckName(res_), 0);
  ostream &rvs = tab_.ResourceValueSectionStream();
  ostream &is = tab_.InitialValueSectionStream();
  is << "      " << RegMailboxName(res_) << " <= 0;\n"
     << "      " << RegMailboxPutAckName(res_) << " <= 0;\n"
     << "      " << RegMailboxGetAckName(res_) << " <= 0;\n";
  string wrn = GetNameRW(res_, true);
  rvs << "  assign " << wire::Names::ResourceWire(wrn, "put_ack")
      << " = " << RegMailboxPutAckName(res_) << ";\n";
  string rrn = GetNameRW(res_, false);
  rvs << "  assign " << wire::Names::ResourceWire(rrn, "get_ack")
      << " = " << RegMailboxGetAckName(res_) << ";\n";
  ostream &os = tab_.StateOutputSectionStream();
  string put_cond = wire::Names::ResourceWire(wrn, "put_req")
    + " && !" + RegMailboxName(res_);
  os << "      " << RegMailboxPutAckName(res_) << " <= "
     << put_cond << ";\n";

  string get_req = wire::Names::ResourceWire(rrn, "get_req");
  string get_cond = get_req + " && " + RegMailboxName(res_);
  os << "      " << RegMailboxGetAckName(res_) << " <= "
     << get_cond << ";\n";
  os << "      if (" << RegMailboxName(res_) << ") begin\n"
     << "        " << RegMailboxName(res_) << " <= "
     << "!(" << get_req << ");\n"
     << "      end else begin\n"
     << "        " << RegMailboxName(res_) << " <= "
     << put_cond << ";\n"
     << "      end\n";
}

void SharedReg::BuildNotifier() {
  tab_.AddReg(RegNotifierName(res_), 0);
  ostream &os = tab_.StateOutputSectionStream();
  os << "      " << RegNotifierName(res_)
     << " <= ";
  string wrn = GetNameRW(res_, true);
  os << wire::Names::ResourceWire(wrn, "notify")
     << ";\n";
  ostream &is = tab_.InitialValueSectionStream();
  is << "      " << RegNotifierName(res_) << " <= 0;\n";
  if (readers_.size() > 0) {
    ostream &rvs = tab_.ResourceValueSectionStream();
    string rrn = GetNameRW(res_, false);
    rvs << "  assign " << wire::Names::ResourceWire(rrn, "notify")
	<< " = " << RegNotifierName(res_) << ";\n";
  }
}

void SharedReg::BuildInsn(IInsn *insn, State *st) {
  auto *klass = res_.GetClass();
  if (resource::IsSharedReg(*klass)) {
    if (!need_write_arbitration_ &&
	insn->inputs_.size() == 1) {
      ostream &os = st->StateBodySectionStream();
      os << "          " << RegName(res_) << " <= "
	 << InsnWriter::RegisterValue(*insn->inputs_[0], tab_.GetNames())
	 << ";\n";
    }
    if (insn->outputs_.size() == 1) {
      // Read from self.
      ostream &ws = tab_.InsnWireValueSectionStream();
      ws << "  assign "
	 << InsnWriter::InsnOutputWireName(*insn, 0)
	 << " = "
	 << RegName(res_) << ";\n";
    }
  }
}

string SharedReg::RegNotifierName(const IResource &reg) {
  return RegName(reg) + "_notify";
}

string SharedReg::RegMailboxName(const IResource &reg) {
  return RegName(reg) + "_mailbox";
}

string SharedReg::RegMailboxPutAckName(const IResource &writer) {
  return RegName(writer) + "_mailbox_put_ack";
}

string SharedReg::RegMailboxGetAckName(const IResource &reader) {
  return RegName(reader) + "_mailbox_get_ack";
}

string SharedReg::RegMailboxBufName(const IResource &reader) {
  return RegName(reader) + "_mailbox_buf";
}

string SharedReg::RegName(const IResource &reg) {
  auto *params = reg.GetParams();
  int unused_width;
  string port_name;
  if (params != nullptr) {
    params->GetExtOutputPort(&port_name, &unused_width);
  }
  string n = GetName(reg);
  if (!port_name.empty()) {
    n += "_" + port_name;
  }
  return n;
}

string SharedReg::GetName(const IResource &reg) {
  ITable *tab = reg.GetTable();
  IModule *mod = tab->GetModule();
  return "shared_reg_" +
    Util::Itoa(mod->GetId()) + "_" +
    Util::Itoa(tab->GetId()) + "_" + Util::Itoa(reg.GetId());
}

string SharedReg::GetNameRW(const IResource &reg, bool is_write) {
  string s = GetName(reg);
  if (is_write) {
    return s + "_w";
  } else {
    return s + "_r";
  }
}

void SharedReg::BuildAccessorWireR() {
  auto &conn = tab_.GetModule()->GetConnection();
  auto &readers = conn.GetSharedRegReaders(&res_);
  wire::WireSet *ws = new wire::WireSet(*this, GetNameRW(res_, false));
  int dw = res_.GetParams()->GetWidth();
  for (auto *reader : readers) {
    wire::AccessorInfo *ainfo = ws->AddAccessor(reader);
    ainfo->SetDistance(reader->GetParams()->GetDistance());
    ainfo->AddSignal("r", wire::AccessorSignalType::ACCESSOR_READ_ARG, dw);
    if (SharedRegAccessor::UseNotify(reader)) {
      ainfo->AddSignal("notify",
		       wire::AccessorSignalType::ACCESSOR_NOTIFY_ACCESSOR, 0);
    }
    if (SharedRegAccessor::UseMailbox(reader)) {
      ainfo->AddSignal("get_req", wire::AccessorSignalType::ACCESSOR_REQ, 0);
      ainfo->AddSignal("get_ack", wire::AccessorSignalType::ACCESSOR_ACK, 0);
    }
  }
  ws->Build();
}

void SharedReg::BuildAccessorWireW() {
  wire::WireSet *ws = new wire::WireSet(*this, GetNameRW(res_, true));
  int dw = res_.GetParams()->GetWidth();
  for (auto *writer : writers_) {
    wire::AccessorInfo *ainfo = ws->AddAccessor(writer);
    ainfo->SetDistance(writer->GetParams()->GetDistance());
    ainfo->AddSignal("w", wire::AccessorSignalType::ACCESSOR_WRITE_ARG, dw);
    ainfo->AddSignal("wen",
		     wire::AccessorSignalType::ACCESSOR_NOTIFY_PARENT, 0);
    auto *klass = writer->GetClass();
    if ((resource::IsSharedRegWriter(*klass) &&
	 SharedRegAccessor::UseNotify(writer)) ||
	(resource::IsSharedRegExtWriter(*klass) &&
	 SharedRegExtWriter::UseNotify(writer))) {
      ainfo->AddSignal("notify",
		       wire::AccessorSignalType::ACCESSOR_NOTIFY_PARENT_SECONDARY, 0);
    }
    if ((resource::IsSharedRegWriter(*klass) &&
	 SharedRegAccessor::UseMailbox(writer)) ||
	(resource::IsSharedRegExtWriter(*klass) &&
	 SharedRegExtWriter::UseMailbox(writer))) {
      ainfo->AddSignal("put_req", wire::AccessorSignalType::ACCESSOR_REQ, 0);
      ainfo->AddSignal("put_ack", wire::AccessorSignalType::ACCESSOR_ACK, 0);
    }
  }
  ws->Build();
}

void SharedReg::GetOptions(bool *use_notify, bool *use_mailbox) {
  *use_notify = false;
  *use_mailbox = false;
  for (auto *reader : readers_) {
    if (resource::IsDataFlowIn(*(reader->GetClass()))) {
      *use_notify |= true;
    } else {
      bool n, m;
      SharedRegAccessor::GetAccessorFeatures(reader, &n, &m);
      *use_notify |= n;
      *use_mailbox |= m;
    }
  }
  for (auto *writer : writers_) {
    bool n, m;
    if (resource::IsSharedRegExtWriter(*(writer->GetClass()))) {
      SharedRegExtWriter::GetAccessorFeatures(writer, &n, &m);
    } else {
      // shared-reg-writer.
      SharedRegAccessor::GetAccessorFeatures(writer, &n, &m);
    }
    *use_notify |= n;
    *use_mailbox |= m;
  }
}

}  // namespace verilog
}  // namespace writer
}  // namespace iroha
