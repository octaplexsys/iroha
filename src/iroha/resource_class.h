// -*- C++ -*-
#ifndef _iroha_resource_class_h_
#define _iroha_resource_class_h_

#include "iroha/common.h"

namespace iroha {
namespace resource {
// NOTE(yt76): Enums instead of string?
const char kTransition[] = "tr";
const char kSet[] = "set";
const char kPrint[] = "print";
const char kAssert[] = "assert";
const char kChannelWrite[] = "channel_write";
const char kChannelRead[] = "channel_read";
const char kSubModuleTaskCall[] = "sub_module_task_call";
const char kSubModuleTask[] = "sub_module_task";
const char kExtInput[] = "ext_input";
const char kExtOutput[] = "ext_output";
const char kArray[] = "array";
const char kEmbedded[] = "embedded";
const char kAdd[] = "add";
const char kSub[] = "sub";
const char kXor[] = "xor";
const char kGt[] = "gt";
const char kShift[] = "shift";

bool IsExclusiveBinOp(const IResourceClass &rc);
bool IsNumToNumExclusiveBinOp(const IResourceClass &rc);
bool IsNumToBoolExclusiveBinOp(const IResourceClass &rc);
bool IsLightBinOp(const IResourceClass &rc);
bool IsBitArrangeOp(const IResourceClass &rc);
bool IsArray(const IResourceClass &rc);
bool IsSubModuleTaskCall(const IResourceClass &rc);
bool IsSubModuleTask(const IResourceClass &rc);

void InstallResourceClasses(IDesign *design);

}  // namespace resource
}  // namespace iroha

#endif  // _iroha_resource_class_h_