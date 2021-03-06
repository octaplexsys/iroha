#include "iroha/resource_params.h"

#include "iroha/logging.h"
#include "iroha/stl_util.h"

#include <set>

static const string boolToStr(bool b) {
  if (b) {
    return "true";
  } else {
    return "false";
  }
}

namespace iroha {
namespace resource {

class ResourceParamValue {
public:
  string key_;
  vector <string> values_;
};

class ResourceParamValueSet {
public:
  ~ResourceParamValueSet() {
    STLDeleteValues(&params_);
  }
  void Merge(ResourceParamValueSet *src_param_values);
  ResourceParamValue *GetParam(const string &key,
			       bool cr,
			       const string &dflt);
  ResourceParamValue *LookupParam(const string &key) const;
  vector<string> GetParamKeys() const;
  vector<string> GetValues(const string &key) const;
  void SetValues(const string &key, const vector<string> &values);
  bool GetBoolParam(const string &key,
		    bool dflt) const;
  void SetBoolParam(const string &key, bool value);
  string GetStringParam(const string &key,
			const string &dflt) const;
  void SetStringParam(const string &key, const string &value);
  int GetIntParam(const string &key, int dflt) const;
  void SetIntParam(const string &key, int value);

  vector<ResourceParamValue *> params_;
};

void ResourceParamValueSet::Merge(ResourceParamValueSet *src_param_values) {
  vector<ResourceParamValue *> new_params;
  std::set<string> keys;
  for (auto *v : src_param_values->params_) {
    ResourceParamValue *nv = new ResourceParamValue();
    *nv = *v;
    new_params.push_back(nv);
    keys.insert(v->key_);
  }
  for (auto *v : params_) {
    if (keys.find(v->key_) == keys.end()) {
      new_params.push_back(v);
    } else {
      // overwritten.
      delete v;
    }
  }
  params_ = new_params;
}

ResourceParamValue *ResourceParamValueSet::GetParam(const string &key,
						    bool cr,
						    const string &dflt) {
  ResourceParamValue *p = LookupParam(key);
  if (p != nullptr) {
    return p;
  }
  if (!cr) {
    return nullptr;
  }
  p = new ResourceParamValue;
  p->key_ = key;
  if (!dflt.empty()) {
    p->values_.push_back(dflt);
  }
  params_.push_back(p);
  return p;
}

ResourceParamValue *ResourceParamValueSet::LookupParam(const string &key)
  const {
  for (auto *p : params_) {
    if (p->key_ == key) {
      return p;
    }
  }
  return nullptr;
}

vector<string> ResourceParamValueSet::GetParamKeys() const {
  vector<string> v;
  for (auto *p : params_) {
    v.push_back(p->key_);
  }
  return v;
}

vector<string> ResourceParamValueSet::GetValues(const string &key) const {
  for (auto *p : params_) {
    if (p->key_ == key) {
      return p->values_;
    }
  }
  vector<string> v;
  return v;
}

void ResourceParamValueSet::SetValues(const string &key,
				      const vector<string> &values) {
  auto *param = GetParam(key, true, "");
  param->values_ = values;
}

bool ResourceParamValueSet::GetBoolParam(const string &key,
					 bool dflt) const {
  ResourceParamValue *p = LookupParam(key);
  if (p == nullptr) {
    return dflt;
  }
  if (p->values_.size() > 0 && Util::ToLower(p->values_[0]) == "true") {
    return true;
  }
  return false;
}

void ResourceParamValueSet::SetBoolParam(const string &key, bool value) {
  auto *param = GetParam(key, true, boolToStr(value));
  param->values_.clear();
  param->values_.push_back(boolToStr(value));
}

string ResourceParamValueSet::GetStringParam(const string &key,
					     const string &dflt) const {
  ResourceParamValue *p = LookupParam(key);
  if (p == nullptr) {
    return dflt;
  }
  if (p->values_.size() > 0) {
    return p->values_[0];
  }
  return string();
}

void ResourceParamValueSet::SetStringParam(const string &key,
					   const string &value) {
  auto *param = GetParam(key, true, "");
  if (param == nullptr) {
    CHECK(value.empty()) << "param should be non null if value is not empty";
    return;
  }
  param->values_.clear();
  param->values_.push_back(value);
}

int ResourceParamValueSet::GetIntParam(const string &key,
				       int dflt) const {
  ResourceParamValue *p = LookupParam(key);
  if (p == nullptr) {
    return dflt;
  }
  if (p->values_.size() > 0) {
    return Util::Atoi(p->values_[0]);
  }
  return dflt;
}

void ResourceParamValueSet::SetIntParam(const string &key, int value) {
  auto *param = GetParam(key, true, Util::Itoa(value));
  param->values_.clear();
  param->values_.push_back(Util::Itoa(value));
}

}  // namespace resource

ResourceParams::ResourceParams()
  : values_(new resource::ResourceParamValueSet) {
}

ResourceParams::~ResourceParams() {
  delete values_;
}

void ResourceParams::Merge(ResourceParams *src_params) {
  values_->Merge(src_params->values_);
}

vector<string> ResourceParams::GetParamKeys() const {
  return values_->GetParamKeys();
}

vector<string> ResourceParams::GetValues(const string &key) const {
  return values_->GetValues(key);
}

void ResourceParams::SetValues(const string &key,
			       const vector<string> &values) {
  values_->SetValues(key, values);
}

string ResourceParams::GetModuleNamePrefix() const {
  return values_->GetStringParam(resource::kModuleNamePrefix, "");
}

void ResourceParams::SetModuleNamePrefix(const string &prefix) {
  values_->SetStringParam(resource::kModuleNamePrefix, prefix);
}

string ResourceParams::GetPortNamePrefix() const {
  return values_->GetStringParam(resource::kPortNamePrefix, "");
}

void ResourceParams::SetPortNamePrefix(const string &prefix) {
  values_->SetStringParam(resource::kPortNamePrefix, prefix);
}

string ResourceParams::GetSramPortIndex() const {
  return values_->GetStringParam(resource::kSramPortIndex, "");
}

void ResourceParams::SetSramPortIndex(const string &idx) {
  values_->SetStringParam(resource::kSramPortIndex, idx);
}

bool ResourceParams::GetResetPolarity() const {
  // default is negative.
  return values_->GetBoolParam(resource::kResetPolarity, false);
}

void ResourceParams::SetResetPolarity(bool p) {
  values_->SetBoolParam(resource::kResetPolarity, p);
}

string ResourceParams::GetPlatformFamily() const {
  return values_->GetStringParam(resource::kPlatformFamily,
				 "generic-platform");
}

void ResourceParams::SetPlatformFamily(const string &name) {
  values_->SetStringParam(resource::kPlatformFamily, name);
}

string ResourceParams::GetPlatformName() const {
  return values_->GetStringParam(resource::kPlatformName, "");
}

void ResourceParams::SetPlatformName(const string &name) {
  values_->SetStringParam(resource::kPlatformName, name);
}

int ResourceParams::GetMaxDelayPs() const {
  // default to 100MHz.
  return values_->GetIntParam(resource::kMaxDelayPs, 10000);
}

void ResourceParams::SetMaxDelayPs(int ps) {
  values_->SetIntParam(resource::kMaxDelayPs, ps);
}

bool ResourceParams::HasResetPolarity() const {
  return (values_->LookupParam(resource::kResetPolarity) != nullptr);
}

string ResourceParams::GetResetName() const {
  return values_->GetStringParam(resource::kResetName, "");
}

void ResourceParams::SetResetName(const string &name) {
  values_->SetStringParam(resource::kResetName, name);
}

int ResourceParams::GetAddrWidth() const {
  return values_->GetIntParam(resource::kAddrWidth, 32);
}

void ResourceParams::SetAddrWidth(int width) {
  values_->SetIntParam(resource::kAddrWidth, width);
}

void ResourceParams::SetExtInputPort(const string &input, int width) {
  values_->SetStringParam(resource::kExtInputPort, input);
  values_->SetIntParam(resource::kExtIOWidth, width);
}

void ResourceParams::GetExtInputPort(string *name, int *width) {
  *name = values_->GetStringParam(resource::kExtInputPort, "");
  if (width != nullptr) {
    *width = values_->GetIntParam(resource::kExtIOWidth, 0);
  }
}

void ResourceParams::SetExtOutputPort(const string &output, int width) {
  values_->SetStringParam(resource::kExtOutputPort, output);
  values_->SetIntParam(resource::kExtIOWidth, width);
}

void ResourceParams::GetExtOutputPort(string *name, int *width) {
  *name = values_->GetStringParam(resource::kExtOutputPort, "");
  if (width != nullptr) {
    *width = values_->GetIntParam(resource::kExtIOWidth, 0);
  }
}

bool ResourceParams::GetInitialValue(int *value) const {
  auto *p = values_->LookupParam(resource::kInitialValue);
  if (p == nullptr || p->values_.size() == 0) {
    return false;
  }
  *value = Util::Atoi(p->values_[0]);
  return true;
}

void ResourceParams::SetInitialValue(int value) {
  values_->SetIntParam(resource::kInitialValue, value);
}

bool ResourceParams::GetDefaultValue(int *value) const {
  auto *p = values_->LookupParam(resource::kDefaultOutputValue);
  if (p == nullptr || p->values_.size() == 0) {
    return false;
  }
  *value = Util::Atoi(p->values_[0]);
  return true;
}

void ResourceParams::SetDefaultValue(int value) {
  values_->SetIntParam(resource::kDefaultOutputValue, value);
}

int ResourceParams::GetWidth() {
  return values_->GetIntParam(resource::kExtIOWidth, 0);
}

void ResourceParams::SetWidth(int w) {
  values_->SetIntParam(resource::kExtIOWidth, w);
}

void ResourceParams::SetEmbeddedModuleName(const string &mod,
					   const string &fn) {
  values_->SetStringParam(resource::kEmbeddedModule, mod);
  values_->SetStringParam(resource::kEmbeddedModuleFile, fn);
}

string ResourceParams::GetEmbeddedModuleName() const {
  return values_->GetStringParam(resource::kEmbeddedModule, "");
}

string ResourceParams::GetEmbeddedModuleFileName() const {
  return values_->GetStringParam(resource::kEmbeddedModuleFile, "");
}

string ResourceParams::GetEmbeddedModuleClk() const {
  return values_->GetStringParam(resource::kEmbeddedModuleClk, "clk");
}

string ResourceParams::GetEmbeddedModuleReset() const {
  return values_->GetStringParam(resource::kEmbeddedModuleReset, "rst");
}

void ResourceParams::SetEmbeddedModuleIO(bool is_output, const vector<string> &ports) {
  if (is_output) {
    SetValues(resource::kEmbeddedModuleOutputs, ports);
  } else {
    SetValues(resource::kEmbeddedModuleInputs, ports);
  }
}

vector<string> ResourceParams::GetEmbeddedModuleIO(bool is_output) {
  if (is_output) {
    return GetValues(resource::kEmbeddedModuleOutputs);
  } else {
    return GetValues(resource::kEmbeddedModuleInputs);
  }
}

string ResourceParams::GetExtTaskName() const {
  return values_->GetStringParam(resource::kExtTaskName, "");
}

void ResourceParams::SetExtTaskName(const string &name) {
  values_->SetStringParam(resource::kExtTaskName, name);
}

int ResourceParams::GetDistance() const {
  return values_->GetIntParam(resource::kDistance, 0);
}

void ResourceParams::SetDistance(int distance) {
  return values_->SetIntParam(resource::kDistance, distance);
}

void ResourceParams::SetWenSuffix(const string &wen) {
  values_->SetStringParam(resource::kWenSuffix, wen);
}

string ResourceParams::GetWenSuffix() const {
  return values_->GetStringParam(resource::kWenSuffix, "");
}

void ResourceParams::SetNotifySuffix(const string &notify) {
  values_->SetStringParam(resource::kNotifySuffix, notify);
}

string ResourceParams::GetNotifySuffix() const {
  return values_->GetStringParam(resource::kNotifySuffix, "");
}

void ResourceParams::SetPutSuffix(const string &put) {
  values_->SetStringParam(resource::kPutSuffix, put);
}

string ResourceParams::GetPutSuffix() {
  return values_->GetStringParam(resource::kPutSuffix, "");
}

int ResourceParams::GetLoopUnroll() {
  return values_->GetIntParam(resource::kLoopUnroll, 1);
}

void ResourceParams::SetLoopUnroll(int unroll) {
  return values_->SetIntParam(resource::kLoopUnroll, unroll);
}

bool ResourceParams::GetIsPipeline() {
  return values_->GetBoolParam(resource::kPipeline, false);
}

void ResourceParams::SetIsPipeline(bool pipeline) {
  values_->SetBoolParam(resource::kPipeline, pipeline);
}

}  // namespace iroha
