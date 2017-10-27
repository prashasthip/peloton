#include "udf/ast_nodes.h"
#include <iostream>  // Remove later , for couts
#include "codegen/type/type.h"
#include "type/types.h"

namespace peloton {
namespace udf {

// Codegen for NumberExprAST
peloton::codegen::Value NumberExprAST::Codegen(
    peloton::codegen::CodeGen &codegen,
    UNUSED_ATTRIBUTE peloton::codegen::FunctionBuilder &fb) {
  std::cout << "NumberExprAST codegen\n";
  auto number_codegen_val = peloton::codegen::Value(
      peloton::codegen::type::Type(type::TypeId::DECIMAL, false),
      codegen.ConstDouble(val));
  return number_codegen_val;
}

// Codegen for VariableExprAST
peloton::codegen::Value VariableExprAST::Codegen(
    UNUSED_ATTRIBUTE peloton::codegen::CodeGen &codegen,
    peloton::codegen::FunctionBuilder &fb) {
  std::cout << "VariabelExprAST codegen\n";
  llvm::Value *val = fb.GetArgumentByName(name);

  if (val) {
    return peloton::codegen::Value(
        peloton::codegen::type::Type(type::TypeId::DECIMAL, false), val);
  }
  return LogErrorV("Unknown variable name");
}

// Codegen for BinaryExprAST
peloton::codegen::Value BinaryExprAST::Codegen(
    peloton::codegen::CodeGen &codegen,
    peloton::codegen::FunctionBuilder &fb) {
  std::cout << "Binary ExprAST codegen\n";

  peloton::codegen::Value left = lhs->Codegen(codegen, fb);
  peloton::codegen::Value right = rhs->Codegen(codegen, fb);
  if (left.GetValue() == 0 || right.GetValue() == 0) {
    return peloton::codegen::Value();
  }

  switch (op) {
    case '+':
      return left.Add(codegen, right);
    case '-':
      return left.Sub(codegen, right);
    case '*':
      return left.Mul(codegen, right);
    case '/':
      return left.Div(codegen, right);
    // TODO(PP): add < and > later on
    /*case '>':
      L = Builder.CreateFCmpUGT(L, R, "cmpGT");
      // Convert bool 0/1 to double 0.0 or 1.0
      return Builder.CreateUIToFP(L, Type::getDoubleTy(getGlobalContext()),
    "boolGT"); case '<': L = Builder.CreateFCmpULT(L, R, "cmptmp");
      // Convert bool 0/1 to double 0.0 or 1.0
      return Builder.CreateUIToFP(L, Type::getDoubleTy(getGlobalContext()),
                                  "boolLT"); */
    default:
      return LogErrorV("invalid binary operator");
  }
}

// Codegen for CallExprAST
peloton::codegen::Value CallExprAST::Codegen(
    peloton::codegen::CodeGen &codegen,
    peloton::codegen::FunctionBuilder &fb) {
  std::cout << "CallExprAST codegen\n";

  // Check if present in the current code context
  // Else, check the catalog and find it
  auto *callee_func = fb.GetFunction();

  // TODO(PP) : Later change this to also check in the catalog
  if (callee_func == 0) {
    return LogErrorV("Unknown function referenced");
  }

  // If argument mismatch error.
  if (callee_func->arg_size() != args.size())
    return LogErrorV("Incorrect # arguments passed");

  std::vector<llvm::Value *> args_val;
  for (unsigned i = 0, size = args.size(); i != size; ++i) {
    args_val.push_back(args[i]->Codegen(codegen, fb).GetValue());
    if (args_val.back() == 0) {
      return LogErrorV("Arguments could not be passed in");
    }
  }

  auto *call_ret = codegen.CallFunc(callee_func, args_val);

  auto call_val = peloton::codegen::Value(
      peloton::codegen::type::Type(type::TypeId::DECIMAL, false), call_ret);

  return call_val;
}

// Codegen for FunctionAST
llvm::Function *FunctionAST::Codegen(
    peloton::codegen::CodeGen &codegen,
    peloton::codegen::FunctionBuilder &fb) {
  std::cout << "FunctionAST codegen\n";

  peloton::codegen::Value ret = body->Codegen(codegen, fb);

  fb.ReturnAndFinish(ret.GetValue());

  return fb.GetFunction();
}

std::unique_ptr<ExprAST> LogError(const char *str) {
  fprintf(stderr, "Error: %s\n", str);
  return 0;
}

peloton::codegen::Value LogErrorV(const char *str) {
  LogError(str);
  return peloton::codegen::Value();
}

}  // namespace udf
}  // namespace peloton
