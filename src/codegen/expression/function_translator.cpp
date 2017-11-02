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
}

codegen::Value FunctionTranslator::DeriveValue(CodeGen &codegen,
                                               RowBatch::Row &row) const {
  // The function expression
  const auto &func_expr = GetExpressionAs<expression::FunctionExpression>();

  // Collect the arguments for the function
  std::vector<codegen::Value> args;
    for (uint32_t i = 0; i < func_expr.GetChildrenSize(); i++) {
      args.push_back(row.DeriveValue(codegen, *func_expr.GetChild(i)));
  }

  if(!func_expr.isUDF()) {
    auto operator_id = func_expr.GetFunc().op_id;

    if (args.size() == 1) {
      // Call unary operator
      return args[0].CallUnaryOp(codegen, operator_id);
    } else if (args.size() == 2) {
      // It's a binary function
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
  } else {
    // It's a UDF
    auto *func_context = func_expr.GetFuncContext(); //The code_context associated with the UDF

    std::vector<llvm::Value *> raw_args;
    for (uint32_t i = 0; i < args.size(); i++) {
      raw_args.push_back(args[i].GetValue());
      //raw_args.push_back(llvm::ConstantFP::get(func_context->GetUDF()->getFunctionType()->getContext(), llvm::APFloat(1.0)));
    }

    std::cout <<
    func_context->GetUDF()->getFunctionType()->getNumParams() << "\n"; //-> 1

    std::cout << (func_context->GetUDF()->getFunctionType()->getParamType(0));
    std::cout << "\n"; //-> pointer1

    std::cout << raw_args[0]->getType();
    std::cout << "\n"; // -> pointer different

    std::cout << raw_args.size();
    std::cout << "\n"; 

    func_context->GetUDF()->getFunctionType()->getParamType(0)->dump();
    std::cout << "\n"; // -> double

    raw_args[0]->getType()->dump();
    std::cout << "\n"; // -> double

    std::cout << func_context->GetUDF()->getFunctionType()->getParamType(0)->getTypeID();
    std::cout << "\n"; // -> 3

    std::cout << raw_args[0]->getType()->getTypeID();
    std::cout << "\n"; // -> 3

    std::cout << (func_context->GetUDF()->getFunctionType()->getParamType(0) == raw_args[0]->getType());
    std::cout << "\n";
    // -> returns 0. Both return llvm::Type *. From the dumps above, they both return double and the typeIDs match.
    // However the constructor of llvm::Type takes llvm::context and typeID. They are still differing in llvm::contexts.
    // Hence, the assertion error:
    // Assertion failed: ((i >= FTy->getNumParams() || FTy->getParamType(i) == Args[i]->getType()) && "Calling a function with a bad signature!"), function init, file /private/tmp/llvm@3.7-20170130-86139-15td9ho/llvm-3.7.1.src/lib/IR/Instructions.cpp, line 239.

    //Construct the new functionType in this context and use that
    llvm::Type *llvm_ret_type =
      GetCodegenParamType(func_expr.GetValueType(), codegen);

    // vector of pair of <argument name, argument type>
    std::vector<llvm::Type *> llvm_args;

    auto args_type = func_expr.GetArgTypes();
    auto iterator_arg_type = args_type.begin();

    while(iterator_arg_type != args_type.end()) {
      llvm_args.push_back(GetCodegenParamType(*iterator_arg_type, codegen));

      ++iterator_arg_type;
    }

    auto *fn_type = llvm::FunctionType::get(llvm_ret_type, llvm_args, false);

    // Construct the function prototype
    auto *func_ptr = llvm::Function::Create(fn_type,
              llvm::Function::ExternalLinkage, func_expr.GetFuncName(),
              &(codegen.GetCodeContext().GetModule()));

    std::cout << "Created function prototype in new code context\n";

    codegen.GetCodeContext().RegisterExternalFunction(func_ptr, 
      func_context->GetRawFunctionPointer(func_context->GetUDF()));

    std::cout << "Registered External Function\n";
    
    auto call_ret = codegen.CallFunc(func_ptr, raw_args);

    std::cout << "Returning result from UDF\n";
    // Should be changed to accomodate any return type
    return codegen::Value{
      type::Decimal::Instance(), call_ret, nullptr, nullptr};
    }
  }

  llvm::Type *FunctionTranslator::GetCodegenParamType(arg_type type_val,
    peloton::codegen::CodeGen &cg) const{
  // TODO(PP) : Add more types later
  if(type_val == arg_type::INTEGER) {
    std::cout << "integer type\n";
    return cg.Int32Type();
  } else if(type_val == arg_type::DECIMAL) {
    std::cout << "double type\n";
    return cg.DoubleType();
  } else {
    //For now assume it to be a bool to keep compiler happy
    return cg.BoolType();
    }
  }

}  // namespace codegen
}  // namespace peloton
