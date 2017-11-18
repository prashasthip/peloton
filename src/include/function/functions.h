//===----------------------------------------------------------------------===//
//
//                         Peloton
//
// functions.h
//
// Identification: src/include/function/functions.h
//
// Copyright (c) 2015-2017, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <unordered_map>
#include <vector>

#include "type/value.h"
#include "codegen/code_context.h"
#include "codegen/value.h"

namespace peloton {
namespace function {

struct BuiltInFuncType {
  typedef type::Value (*Func)(const std::vector<type::Value> &);

  OperatorId op_id;
  Func impl;

  BuiltInFuncType(OperatorId _op_id, Func _impl) : op_id(_op_id), impl(_impl) {}
  BuiltInFuncType() : BuiltInFuncType(OperatorId::Invalid, nullptr) {}
};

class BuiltInFunctions {
 private:
  static std::unordered_map<std::string, BuiltInFuncType> kFuncMap;

 public:
  static void AddFunction(const std::string &func_name, BuiltInFuncType func);

  static BuiltInFuncType GetFuncByName(const std::string &func_name);
};

typedef uint32_t oid_t;

class PlpgsqlFunctions {
  private:
    static std::unordered_map<oid_t, codegen::CodeContext *> kFuncMap;

  public:
    static void AddFunction(const oid_t oid,
      codegen::CodeContext *func_context);

    static codegen::CodeContext *GetFuncContextByOid(const oid_t oid);
};

}  // namespace function
}  // namespace peloton
