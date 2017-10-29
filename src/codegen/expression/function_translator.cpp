//===----------------------------------------------------------------------===//
//
//                         Peloton
//
// function_translator.cpp
//
// Identification: src/codegen/expression/function_translator.cpp
//
// Copyright (c) 2015-2017, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "codegen/expression/function_translator.h"

#include "codegen/compilation_context.h"
#include "codegen/type/sql_type.h"
#include "codegen/type/type_system.h"
#include "expression/function_expression.h"
#include "codegen/type/decimal_type.h"

namespace peloton {
namespace codegen {

FunctionTranslator::FunctionTranslator(
    const expression::FunctionExpression &func_expr,
    CompilationContext &context)
    : ExpressionTranslator(func_expr, context) {
  if(!func_expr.isUDF()) {
    PL_ASSERT(func_expr.GetFunc().op_id != OperatorId::Invalid);
    PL_ASSERT(func_expr.GetFunc().impl != nullptr);
  } else {
    PL_ASSERT(func_expr.GetFuncContext() != nullptr);
  }

  // Prepare each of the child expressions
  for (uint32_t i = 0; i < func_expr.GetChildrenSize(); i++) {
    context.Prepare(*func_expr.GetChild(i));
  }

  std::cout << "inside the FunctionTranslator() constructor\n";
}

codegen::Value FunctionTranslator::DeriveValue(CodeGen &codegen,
                                               RowBatch::Row &row) const {
  std::cout << "Inside FunctionTranslator::DeriveValue()\n";

  // The function expression
  const auto &func_expr = GetExpressionAs<expression::FunctionExpression>();

  std::cout << "After getting function_expr\n";

  // Collect the arguments to the function

  std::cout << "outside here\n";

  if(!func_expr.isUDF()) {
    std::vector<codegen::Value> args;
    for (uint32_t i = 0; i < func_expr.GetChildrenSize(); i++) {
      args.push_back(row.DeriveValue(codegen, *func_expr.GetChild(i)));
    }
    auto operator_id = func_expr.GetFunc().op_id;

    if (args.size() == 1) {
      // Call unary operator
      std::cout << "CallUnaryOp\n";
      return args[0].CallUnaryOp(codegen, operator_id);
    } else if (args.size() == 2) {
      // It's a binary function
      std::cout << "BinaryOp\n";
      // Lookup the function
      type::Type left_type = args[0].GetType(), right_type = args[1].GetType();
      auto *binary_op = type::TypeSystem::GetBinaryOperator(
          operator_id, left_type, left_type, right_type, right_type);
      PL_ASSERT(binary_op);

      // Invoke
      return binary_op->DoWork(codegen, args[0].CastTo(codegen, left_type),
                               args[1].CastTo(codegen, right_type),
                               OnError::Exception);
    } else {
      // It's an N-Ary function

      // Collect argument types for lookup
      std::vector<type::Type> arg_types;
      for (const auto &arg_val : args) {
        arg_types.push_back(arg_val.GetType());
      }

      // Lookup the function
      auto *nary_op = type::TypeSystem::GetNaryOperator(operator_id, arg_types);
      PL_ASSERT(nary_op != nullptr);
      return nary_op->DoWork(codegen, args, OnError::Exception);
    }
  } else { // It's a UDF

    std::cout << "hi\n";
    auto *func_context = func_expr.GetFuncContext();
    CodeGen cg{*func_context};

    std::cout << "bye\n";
    std::vector<codegen::Value> args;
    for (uint32_t i = 0; i < func_expr.GetChildrenSize(); i++) {
      args.push_back(row.DeriveValue(cg, *func_expr.GetChild(i)));
    }

    std::cout << "Evaluating UDF\n";

    std::cout << "Num of args " << args.size() << "\n";

    std::vector<llvm::Value *> raw_args;
    for (uint32_t i = 0; i < args.size(); i++) {
      raw_args.push_back(args[i].GetValue());
      //raw_args.push_back(llvm::ConstantFP::get(func_context->GetUDF()->getFunctionType()->getContext(), llvm::APFloat(1.0)));
    }
    std::cout << "len " << raw_args.size() << "\n";

    std::cout <<
      func_context->GetUDF()->getFunctionType()->getNumParams() << "\n";

      /*std::cout << (func_context->GetUDF()->getFunctionType()->getParamType(0));
      std::cout << "\n";

      std::cout << raw_args[0]->getType();
      std::cout << "\n";

      std::cout << func_context->GetUDF()->getFunctionType()->getParamType(1);
      std::cout << "\n"; */

    func_context->GetUDF()->getFunctionType()->getParamType(0)->dump();
    std::cout << "\n";

    raw_args[0]->getType()->dump();
    std::cout << "\n";

    std::cout << (func_context->GetUDF()->getFunctionType()->getParamType(0) == raw_args[0]->getType());
    std::cout << "\n";

    auto call_ret = func_context->GetBuilder().CreateCall(
        func_context->GetUDF(), raw_args);

    std::cout << "Returning result from UDF\n";
    // Should be changed to accomodate any return type
    return codegen::Value{
      type::Decimal::Instance(), call_ret, nullptr, nullptr};
  }

}

}  // namespace codegen
}  // namespace peloton
