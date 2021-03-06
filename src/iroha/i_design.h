// -*- C++ -*-
#ifndef _iroha_i_design_h_
#define _iroha_i_design_h_

#include "iroha/common.h"
#include "numeric/numeric.h"

namespace iroha {

namespace opt {
class DebugAnnotation;
}  // namespace opt

class OptAPI;
class WriterAPI;

// Type of resource.
// e.g. 'adder' is a resource class and '32 bit adder' is a resource.
class IResourceClass {
public:
  IResourceClass(const string &name, bool is_exclusive, IDesign *design);
  IDesign *GetDesign();
  bool IsExclusive();
  const string &GetName() const;

private:
  const string name_;
  IDesign *design_;
  bool is_exclusive_;
};

class IValueType : public NumericWidth {
public:
  static IValueType FromNumericWidth(const NumericWidth &w);
};

class IArrayImage {
public:
  IArrayImage(IDesign *design);

  IDesign *GetDesign() const;
  int GetId() const;
  void SetId(int id);
  const string &GetName() const;
  void SetName(const string &name);

  vector<uint64_t> values_;
private:
  IDesign *design_;
  int id_;
  string name_;
};

class IArray {
public:
  IArray(IResource *res, int address_width, const IValueType &data_type,
	 bool is_external, bool is_ram);

  // This indicates just ownership of the memory. IArray instance can be
  // shared by many IResources.
  IResource *GetResource() const;
  int GetAddressWidth() const;
  const IValueType &GetDataType() const;
  bool IsExternal() const;
  void SetExternal(bool is_external);
  bool IsRam() const;
  void SetArrayImage(IArrayImage *image);
  IArrayImage *GetArrayImage() const;

private:
  IResource *res_;
  int address_width_;
  IValueType data_type_;
  bool is_external_;
  bool is_ram_;
  IArrayImage *array_image_;
};

class IResource {
public:
  IResource(ITable *table, IResourceClass *resource_class);
  ~IResource();
  ITable *GetTable() const;
  int GetId() const;
  void SetId(int id);
  IResourceClass *GetClass() const;
  ResourceParams *GetParams() const;
  IArray *GetArray() const;
  void SetArray(IArray *array);
  ITable *GetCalleeTable() const;
  void SetCalleeTable(ITable *table);
  IResource *GetParentResource() const;
  void SetParentResource(IResource *res);

  vector<IValueType> input_types_;
  vector<IValueType> output_types_;

private:
  ITable *table_;
  IResourceClass *resource_class_;
  ResourceParams *params_;
  int id_;
  IArray *array_;
  ITable *callee_table_;
  IResource *parent_resource_;
};

class IRegister {
public:
  IRegister(ITable *table, const string &name);
  ITable *GetTable() const;
  int GetId() const;
  void SetId(int id);
  const string &GetName() const;
  void SetName(const string &name);

  void SetInitialValue(Numeric &value);
  void ClearInitialValue();
  const Numeric &GetInitialValue() const;
  bool HasInitialValue() const;
  void SetConst(bool c);
  bool IsConst() const;
  void SetStateLocal(bool s);
  bool IsStateLocal() const;
  // For convenience (!IsStateLocal() && !IsConst()).
  bool IsNormal() const;
  ResourceParams *GetParams(bool cr);

  IValueType value_type_;

private:
  ITable *table_;
  string name_;
  int id_;
  Numeric initial_value_;
  bool has_initial_value_;
  bool is_const_;
  bool state_local_;
  unique_ptr<ResourceParams> params_;
};

class IInsn {
public:
  IInsn(IResource *resource);

  IResource *GetResource() const;
  void SetResource(IResource *resource);
  int GetId() const;
  void SetId(int id);
  const string &GetOperand() const;
  void SetOperand(const string &opr);

  vector<IRegister *> inputs_;
  vector<IRegister *> outputs_;
  vector<IState *> target_states_;
  vector<IInsn *> depending_insns_;

private:
  IResource *resource_;
  int id_;
  string operand_;
};

// Profiling information for an IState. This object is embedded in
// the owner IState.
class IProfile {
public:
  IProfile();

  bool valid_;
  // Raw execution count from the front end.
  long raw_count_;
  // log2 of normalized count.
  long normalized_count_;
};

// This can be a state in an FSM or a pipeline stage.
class IState {
public:
  IState(ITable *table);
  ITable *GetTable() const;
  int GetId() const;
  void SetId(int id);
  IProfile *GetMutableProfile();
  const IProfile &GetProfile() const;

  vector<IInsn *> insns_;

private:
  ITable *table_;
  int id_;
  IProfile profile_;
};

// Table is a matrix of resource columns and state rows to define
// execution scheduling.
// It can mean either a pipeline stage or an FSM.
class ITable {
public:
  ITable(IModule *module);
  IModule *GetModule() const;
  int GetId() const;
  void SetId(int id);
  string GetName() const;
  void SetName(const string &name);
  void SetInitialState(IState *state);
  IState *GetInitialState() const;

  vector<IState *> states_;
  vector<IResource *> resources_;
  vector<IRegister *> registers_;

private:
  IModule *module_;
  int id_;
  string name_;
  IState *initial_state_;
};

// IModule corresponds to a module in HDL.
class IModule {
public:
  IModule(IDesign *design, const string &name);

  IDesign *GetDesign() const;
  int GetId() const;
  void SetId(int id);
  const string &GetName() const;
  void SetParentModule(IModule *mod);
  IModule *GetParentModule() const;
  ResourceParams *GetParams() const;

  vector<ITable *> tables_;

private:
  IDesign *design_;
  int id_;
  string name_;
  IModule *parent_;
  unique_ptr<ResourceParams> params_;
};

// Represents a whole design including module hierarchy,
// interfaces, some external stuff, configs and so on.
class IDesign {
public:
  IDesign();
  ~IDesign();

  ResourceParams *GetParams() const;
  ObjectPool *GetObjectPool();
  void SetDebugAnnotation(opt::DebugAnnotation *annotation);
  opt::DebugAnnotation *GetDebugAnnotation() const;
  OptAPI *GetOptAPI() const;
  void SetOptAPI(OptAPI *opt_api);
  WriterAPI *GetWriterAPI() const;
  void SetWriterAPI(WriterAPI *writer_api);
  // Make the platform independent (or orphan).
  void ReleasePlatform(IPlatform *platform);
  // Take over the ownership from the previous owner (might be null).
  void ManagePlatform(IPlatform *platform);

  vector<IModule *> modules_;
  vector<IResourceClass *> resource_classes_;
  vector<IArrayImage *> array_images_;
  vector<IPlatform *> platforms_;

private:
  ObjectPool *objects_;
  ResourceParams *params_;
  opt::DebugAnnotation *annotation_;

  unique_ptr<OptAPI> opt_api_;
  unique_ptr<WriterAPI> writer_api_;
};

}  // namespace iroha

#endif  // _iroha_i_design_h_
