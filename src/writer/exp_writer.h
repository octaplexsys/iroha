// -*- C++ -*-
#ifndef _writer_exp_writer_h_
#define _writer_exp_writer_h_

#include "iroha/common.h"

namespace iroha {
namespace writer {

class ExpWriter {
public:
  ExpWriter(const IDesign *design, ostream &os);

  void Write();

private:
  void WriteHeader();
  void WriteModule(const IModule &mod);
  void WriteTable(const ITable &tab);
  void WriteInitialState(const ITable &tab);
  void WriteState(const IState &st);
  void WriteInsn(const IInsn &insn);
  void WriteProfile(const IProfile &profile);
  void WriteRegisters(const ITable &tab);
  void WriteResources(const ITable &tab);
  void WriteResource(const IResource &res);
  void WriteArrayDesc(const IResource &res);
  void WriteCalleeTaskDesc(const IResource &res);
  void WriteParentResourceDesc(const IResource &res);
  void WriteValue(const Numeric &value);
  void WriteValueType(const IValueType &type);
  void WriteInsnParams(const vector<IRegister *> &regs);
  void WriteDependingInsns(const vector<IInsn *> &insns);
  void WriteResourceTypes(const vector<IValueType> &types);
  void WriteResourceParams(const ResourceParams &params, const char *indent);
  void WriteArrayImage(const IArrayImage &im);
  void WriteResourceDesc(const IResource &res);
  void WriteStr(const string &str);
  void WritePlatform(const IPlatform &platform);
  void WritePlatformDef(const platform::Definition &def);
  void WritePlatformCondition(const platform::DefNode &cond);
  void WritePlatformValue(const platform::DefNode &value);
  void WriteNodeDef(const platform::DefNode &node);

  const IDesign *design_;
  ostream &os_;
};

}  // namespace writer
}  // namespace iroha

#endif  // _writer_exp_writer_h_
