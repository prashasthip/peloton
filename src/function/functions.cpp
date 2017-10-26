//===----------------------------------------------------------------------===//
//
//                         Peloton
//
// functions.cpp
//
// Identification: src/function/functions.cpp
//
// Copyright (c) 2015-2017, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//


#include "function/functions.h"

namespace peloton {
namespace function {

std::unordered_map<std::string, BuiltInFuncType>
    BuiltInFunctions::func_map;

std::unordered_map<std::string, codegen::CodeContext *>	PlpgsqlUDF::plpgsql_udf_map;

void BuiltInFunctions::AddFunction(const std::string &func_name,
                                   BuiltInFuncType func) {
  func_map.emplace(func_name, func);
}

BuiltInFuncType BuiltInFunctions::GetFuncByName(const std::string &func_name) {
  auto func = func_map.find(func_name);
  if (func == func_map.end())
    return nullptr;
  return func->second;
}

void PlpgsqlUDF::AddFunction(const std::string &func_name,
                            	codegen::CodeContext *code_context) {
  plpgsql_udf_map.emplace(func_name, code_context);
}

codegen::CodeContext *PlpgsqlUDF::GetFuncByName(const std::string &func_name) {
  auto code_cont = plpgsql_udf_map.find(func_name);
  if (code_cont == plpgsql_udf_map.end())
    return nullptr;
  return code_cont->second;
}


}  // namespace function
}  // namespace peloton
