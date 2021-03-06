// -*- C++ -*-
#ifndef _numeric_numeric_manager_h_
#define _numeric_numeric_manager_h_

#include "numeric/numeric.h"
#include "numeric/numeric_width.h"

#include <set>

namespace iroha {

class NumericManager {
public:
  ~NumericManager();

  void MayPopulateStorage(const NumericWidth &w, NumericValue *v);
  void StartGC();
  void MarkStorage(const Numeric *n);
  void DoGC();
  void CopyValue(const Numeric &src, NumericValue *dst);

private:
  std::set<ExtraWideValue *> values_;
  std::set<const ExtraWideValue *> marked_values_;
};

}  // iroha

#endif  // _numeric_numeric_manager_h_
