// -*- C++ -*-
#ifndef _writer_verilog_module_h_
#define _writer_verilog_module_h_

#include "writer/verilog/common.h"

namespace iroha {
namespace writer {
namespace verilog {

// For module.
static const char kStateDeclSection[] = "state_decl";
static const char kEmbeddedInstanceSection[] = "embedded";
static const char kSharedWireSection[] = "shared";
static const char kSubModuleSection[] = "sub_modules";
// For each table (with table id).
static const char kRegisterSection[] = "register";
static const char kResourceSection[] = "resource";
static const char kResourceValueSection[] = "resource_value";
static const char kInsnWireDeclSection[] = "insn_wire_decl";
static const char kInsnWireValueSection[] = "insn_wire_value";
static const char kInitialValueSection[] = "initial_value";
static const char kStateOutputSection[] = "state_output";
static const char kTaskEntrySection[] = "task_entry";
// For each state (with table and state id)
static const char kStateBodySection[] = "state_body";
static const char kStateTransitionSection[] = "state_tr";

class Module {
public:
  Module(const IModule *i_mod, const VerilogWriter *writer,
	 const Connection &conn,
	 EmbeddedModules *embed, Names *names);
  ~Module();

  void Build();
  void PrepareTables();
  void Write(const string &prefix, ostream &os);
  bool GetResetPolarity() const;
  Module *GetParentModule() const;
  void SetParentModule(Module *parent);
  const IModule *GetIModule() const;
  PortSet *GetPortSet() const;
  ModuleTemplate *GetModuleTemplate() const;
  void BuildChildModuleInstSection(vector<Module *> &child_mods);
  const string &GetName() const;
  const Connection &GetConnection() const;
  Names *GetNames() const;
  ostream &ChildModuleInstSectionStream(Module *child) const;
  Module *GetByIModule(const IModule *mod) const;

private:
  string ChildModuleInstSectionContents(Module *child, bool clear) const;

  const IModule *i_mod_;
  const VerilogWriter *writer_;
  const Connection &conn_;
  EmbeddedModules *embed_;
  Names *names_;
  Module *parent_;
  unique_ptr<ModuleTemplate> tmpl_;
  unique_ptr<PortSet> ports_;
  vector<Table *> tables_;
  vector<Module *> child_modules_;
  string name_;
  string reset_name_;
};

}  // namespace verilog
}  // namespace writer
}  // namespace iroha

#endif  // _writer_verilog_module_h_
