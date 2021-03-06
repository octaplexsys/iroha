//  -*- C++ -*-
//
// This class manages names in output file in order to avoid conflits
// between names or against reserved keywords.
// Basic strategies are:
// * Avoid names without '_' (underscore), since they are likely be reserved.
// * Use the substring before first '_' as a prefix and add prefix to the
//   original name, if necessary.
//
#ifndef _writer_names_h_
#define _writer_names_h_

#include "iroha/common.h"

#include <map>
#include <set>

namespace iroha {
namespace writer {

class Names {
public:
  explicit Names(Names *parent);
  ~Names();

  Names *NewChildNames();
  void ReservePrefix(const string &prefix);
  void ReserveGlobalName(const string &name);
  void AssignRegNames(const IModule *mod);
  bool IsReservedName(const string &name);
  bool IsReservedPrefix(const string &prefix);
  string GetRegName(const IRegister &reg);

private:
  string GetPrefix(const string &s);
  string GetName(const string &raw_name, const string &type_prefix);

  Names *parent_;
  set<string> prefixes_;
  set<string> reserved_names_;
  vector<unique_ptr<Names>> child_names_;
  map<const IRegister *, string> reg_names_;
};

}  // namespace writer
}  // namespace iroha

#endif  // _writer_names_h_
