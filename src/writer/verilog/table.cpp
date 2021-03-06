#include "writer/verilog/table.h"

#include "iroha/i_design.h"
#include "iroha/logging.h"
#include "iroha/resource_attr.h"
#include "iroha/resource_class.h"
#include "iroha/resource_params.h"
#include "iroha/stl_util.h"
#include "writer/module_template.h"
#include "writer/names.h"
#include "writer/verilog/embedded_modules.h"
#include "writer/verilog/ext_task.h"
#include "writer/verilog/insn_writer.h"
#include "writer/verilog/module.h"
#include "writer/verilog/port.h"
#include "writer/verilog/port_set.h"
#include "writer/verilog/resource.h"
#include "writer/verilog/state.h"
#include "writer/verilog/task.h"

namespace iroha {
namespace writer {
namespace verilog {

Table::Table(ITable *table, PortSet *ports, Module *mod, EmbeddedModules *embed,
	     Names *names, ModuleTemplate *tmpl)
  : i_table_(table), ports_(ports), mod_(mod), embedded_modules_(embed),
    names_(names), tmpl_(tmpl) {
  table_id_ = table->GetId();
  st_ = "st_" + Util::Itoa(table->GetId());
  is_task_or_ext_task_ = Task::IsTask(*this) || ExtTask::IsExtTask(*this);
}

Table::~Table() {
  STLDeleteValues(&states_);
  STLDeleteSecondElements(&resources_);
}

void Table::CollectNames() {
  for (auto *ires : i_table_->resources_) {
    Resource *res = GetResource(ires);
    res->CollectNames(names_);
  }
}

void Table::Build() {
  BuildStates();

  BuildStateDecl();
  BuildRegister();
  BuildResource();
  BuildInsnOutputWire();
  BuildMultiCycleStateReg();
}

void Table::BuildStates() {
  for (auto *i_state : i_table_->states_) {
    State *st = new State(i_state, this, names_);
    st->Build();
    states_.push_back(st);
  }
}

void Table::BuildStateDecl() {
  if (states_.size() == 0) {
    return;
  }
  ostream &sd = tmpl_->GetStream(kStateDeclSection);
  sd << "  // state names\n";

  int max_id = 0;
  for (auto *st : i_table_->states_) {
    int id = st->GetId();
    sd << "  localparam " << StateName(id) << " = "
       << id << ";\n";
    if (id > max_id) {
      max_id = id;
    }
  }
  if (IsTaskOrExtTask()) {
    ++max_id;
    sd << "  localparam " << StateName(Task::kTaskEntryStateId) << " = "
       << max_id << ";\n";
  }
  int bits = 0;
  int u = 1;
  while (u < max_id) {
    u *= 2;
    ++bits;
  }
  sd << "  reg [" << bits << ":0] " << StateVariable() << ";\n";
  sd << "\n";
}

void Table::BuildResource() {
  for (auto *ires : i_table_->resources_) {
    Resource *res = GetResource(ires);
    if (res != nullptr) {
      res->BuildResource();
    }
  }
}

void Table::BuildRegister() {
  ostream &rs = RegisterSectionStream();
  ostream &is = InitialValueSectionStream();
  for (auto *reg : i_table_->registers_) {
    if (!reg->IsConst()) {
      if (reg->IsStateLocal()) {
	rs << "  wire";
      } else {
	rs << "  reg";
      }
      rs << " " << ValueWidthSpec(reg->value_type_);
      rs << " " << names_->GetRegName(*reg) << "; // "
	 << reg->GetName() << "\n";
    }
    if (!reg->IsConst() && reg->HasInitialValue()) {
      is << "      " << names_->GetRegName(*reg) << " <= "
	 << reg->GetInitialValue().GetValue0() << ";\n";
    }
  }
}

void Table::BuildInsnOutputWire() {
  ostream &rs = InsnWireDeclSectionStream();
  for (IState *st : i_table_->states_) {
    for (IInsn *insn : st->insns_) {
      int nth = 0;
      for (IRegister *oreg : insn->outputs_) {
	rs << "  wire " << ValueWidthSpec(oreg->value_type_) << " "
	   << InsnWriter::InsnOutputWireName(*insn, nth)
	   << ";\n";
	++nth;
      }
    }
  }
}

void Table::BuildMultiCycleStateReg() {
  set<IResource *> mc_resources;
  for (IState *st : i_table_->states_) {
    for (IInsn *insn : st->insns_) {
      if (ResourceAttr::IsMultiCycleInsn(insn)) {
	mc_resources.insert(insn->GetResource());
      }
    }
  }
  ostream &ws = tmpl_->GetStream(kInsnWireDeclSection);
  for (IResource *res : mc_resources) {
    string w = InsnWriter::MultiCycleStateName(*res);
    ws << "  reg [1:0] " << w << ";\n";
    ostream &is = InitialValueSectionStream();
    is << "      " << w << " <= 0;\n";
  }
}

void Table::AddReg(const string &name, int width) const {
  ostream &rs = ResourceSectionStream();
  rs << "  reg " << Table::WidthSpec(width) << name << ";\n";
}

void Table::AddRegWithInitial(const string &name, int width, int value) const {
  AddReg(name, width);
  ostream &is = InitialValueSectionStream();
  is << "      " << name << " <= " << value << ";\n";
}

string Table::ValueWidthSpec(const IValueType &type) {
  if (type.GetWidth() > 0) {
    string s = " [" + Util::Itoa(type.GetWidth() - 1) + ":0]";
    if (type.IsSigned()) {
      s = " signed" + s;
    }
    return s;
  }
  return string();
}

string Table::WidthSpec(int w) {
  string s;
  if (w > 0) {
    s = "[" + Util::Itoa(w - 1) + ":0] ";
  }
  return s;
}

void Table::Write(ostream &os) {
  os << "  // Table " << table_id_ << "\n";
  WriteAlwaysBlockHead(os);
  WriteReset(os);
  WriteAlwaysBlockMiddle(os);
  WriteBody(os);
  WriteAlwaysBlockTail(os);
}

void Table::WriteAlwaysBlockHead(ostream &os) const {
  os << "  always @(posedge " << ports_->GetClk() << ") begin\n"
     << "    if (";
  if (!mod_->GetResetPolarity()) {
    os << "!";
  }
  os << ports_->GetReset() << ") begin\n";
}

void Table::WriteAlwaysBlockMiddle(ostream &os) const {
  os << "    end else begin\n";
}

void Table::WriteAlwaysBlockTail(ostream &os) const {
  os << "    end\n"
     << "  end\n";
}

void Table::WriteReset(ostream &os) {
  if (!IsEmpty()) {
    os << "      " << StateVariable() << " <= ";
    if (IsTaskOrExtTask()) {
      os << StateName(Task::kTaskEntryStateId);
    } else {
      os << InitialStateName();
    }
    os << ";\n";
  }
  os << InitialValueSectionContents();
}

void Table::WriteBody(ostream &os) {
  os << StateOutputSectionContents();
  if (!IsEmpty()) {
    os << "      case (" << StateVariable() << ")\n";
    if (IsTaskOrExtTask()) {
      State::WriteTaskEntry(this, os);
    }
    for (auto *state : states_) {
      state->Write(os);
    }
    os << "      endcase\n";
  }
}

ITable *Table::GetITable() const {
  return i_table_;
}

const string &Table::StateVariable() const {
  return st_;
}

string Table::StateName(int id) const {
  return StateNameFromTable(*i_table_, id);
}

string Table::StateNameFromTable(const ITable &tab, int id) {
  string n = "S_" + Util::Itoa(tab.GetId()) + "_";
  if (id == Task::kTaskEntryStateId) {
    return n + "task_idle";
  } else {
    return n + Util::Itoa(id);
  }
}

ModuleTemplate *Table::GetModuleTemplate() const {
  return tmpl_;
}

Names *Table::GetNames() const {
  return names_;
}

Resource *Table::GetResource(const IResource *ires) {
  auto it = resources_.find(ires);
  if (it != resources_.end()) {
    return it->second;
  }
  Resource *r = Resource::Create(*ires, *this);
  resources_[ires] = r;
  return r;
}

ostream &Table::StateOutputSectionStream() const {
  return tmpl_->GetStream(kStateOutputSection + Util::Itoa(table_id_));
}

string Table::StateOutputSectionContents() const {
  return tmpl_->GetContents(kStateOutputSection + Util::Itoa(table_id_));
}

ostream &Table::InitialValueSectionStream() const {
  return tmpl_->GetStream(kInitialValueSection + Util::Itoa(table_id_));
}

string Table::InitialValueSectionContents() const {
  return tmpl_->GetContents(kInitialValueSection + Util::Itoa(table_id_));
}

ostream &Table::TaskEntrySectionStream() const {
  return tmpl_->GetStream(kTaskEntrySection + Util::Itoa(table_id_));
}

string Table::TaskEntrySectionContents() const {
  return tmpl_->GetContents(kTaskEntrySection + Util::Itoa(table_id_));
}

ostream &Table::ResourceSectionStream() const {
  return tmpl_->GetStream(kResourceSection + Util::Itoa(table_id_));
}

string Table::ResourceSectionContents() const {
  return tmpl_->GetContents(kResourceSection + Util::Itoa(table_id_));
}

ostream &Table::ResourceValueSectionStream() const {
  return tmpl_->GetStream(kResourceValueSection + Util::Itoa(table_id_));
}

string Table::ResourceValueSectionContents() const {
  return tmpl_->GetContents(kResourceValueSection + Util::Itoa(table_id_));
}

ostream &Table::RegisterSectionStream() const {
  return tmpl_->GetStream(kRegisterSection + Util::Itoa(table_id_));
}

string Table::RegisterSectionContents() const {
  return tmpl_->GetContents(kRegisterSection + Util::Itoa(table_id_));
}

ostream &Table::InsnWireDeclSectionStream() const {
  return tmpl_->GetStream(kInsnWireDeclSection + Util::Itoa(table_id_));
}

string Table::InsnWireDeclSectionContents() const {
  return tmpl_->GetContents(kInsnWireDeclSection + Util::Itoa(table_id_));
}

ostream &Table::InsnWireValueSectionStream() const {
  return tmpl_->GetStream(kInsnWireValueSection + Util::Itoa(table_id_));
}

string Table::InsnWireValueSectionContents() const {
  return tmpl_->GetContents(kInsnWireValueSection + Util::Itoa(table_id_));
}
  
string Table::InitialStateName() {
  IState *initial_st = i_table_->GetInitialState();
  if (initial_st == nullptr) {
    LOG(FATAL) << "null initial state.\n";
  }
  return StateName(initial_st->GetId());
}

bool Table::IsTaskOrExtTask() {
  return is_task_or_ext_task_;
}

bool Table::IsEmpty() {
  return (states_.size() == 0);
}

PortSet *Table::GetPortSet() const {
  return ports_;
}

EmbeddedModules *Table::GetEmbeddedModules() const {
  return embedded_modules_;
}

Module *Table::GetModule() const {
  return mod_;
}

Task *Table::GetTask() const {
  return nullptr;
}

string Table::GetStateCondition(const IState *st) const {
  return StateVariable() + " == " +
    StateNameFromTable(*i_table_, st->GetId());
}

const DataFlowTable *Table::GetDataFlowTable() const {
  return nullptr;
}

}  // namespace verilog
}  // namespace writer
}  // namespace iroha
