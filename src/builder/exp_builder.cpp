#include "builder/exp_builder.h"

#include "builder/reader.h"

namespace iroha {

IDesign *ExpBuilder::ReadDesign(const string &fn) {
  iroha::File *f = iroha::Reader::ReadFile(fn);
  if (f->exps.size()) {
    ExpBuilder builder;
    return builder.Build(f->exps);
  }
  return nullptr;
}

ExpBuilder::ExpBuilder() : has_error_(false) {
}

IDesign *ExpBuilder::Build(vector<Exp *> &exps) {
  IDesign *design = new IDesign;
  for (Exp *root : exps) {
    if (root->vec.size() == 0) {
      SetError();
      continue;
    }
    if (root->vec[0]->atom.str == "MODULE") {
      IModule *module = BuildModule(root);
      if (module) {
	design->modules_.push_back(module);
      } else {
	SetError();
      }
    } else {
      SetError();
    }
  }
  if (HasError()) {
    cout << "Build failure";
  }
  return design;
}

IModule *ExpBuilder::BuildModule(Exp *e) {
  if (e->vec.size() < 3) {
    SetError();
    return nullptr;
  }
  IModule *module = new IModule;
  for (int i = 2; i < e->vec.size(); ++i) {
    if (e->vec[i]->vec.size() == 0) {
      SetError();
    } else if (e->vec[i]->vec[0]->atom.str == "TABLE") {
      ITable *table = BuildTable(e->vec[i]);
      if (table) {
	module->tables_.push_back(table);
      } else {
	SetError();
      }
    } else {
      SetError();
    }
  }
  return module;
}

ITable *ExpBuilder::BuildTable(Exp *e) {
  ITable *table = new ITable;
  for (int i = 1; i < e->vec.size(); ++i) {
    if (e->vec[i]->vec[0]->atom.str == "STATE") {
      IState *state = BuildState(e->vec[i]);
      table->states_.push_back(state);
    }
  }
  return table;
}

IState *ExpBuilder::BuildState(Exp *e) {
  return new IState;
}

void ExpBuilder::SetError() {
  has_error_ = true;
}

bool ExpBuilder::HasError() {
  return has_error_;
}

}  // namespace iroha
