#include "udf/ast_nodes.h"
#include <iostream>  // Remove later , for couts
#include "codegen/type/type.h"
#include "type/types.h"

namespace peloton {
namespace udf {

// Keeps track of symbols while declaring a function
static std::map<std::string, llvm::Value *> named_values;

// Codegen for NumberExprAST
peloton::codegen::Value NumberExprAST::Codegen(
    peloton::codegen::CodeGen &codegen) {
  std::cout << "NumberExprAST codegen\n";
  auto number_codegen_val = peloton::codegen::Value(
      peloton::codegen::type::Type(type::TypeId::DECIMAL, false),
      codegen.ConstDouble(val));
  return number_codegen_val;
}

// Codegen for VariableExprAST
peloton::codegen::Value VariableExprAST::Codegen(
    UNUSED_ATTRIBUTE peloton::codegen::CodeGen &codegen) {
  std::cout << "VariabelExprAST codegen\n";
  llvm::Value *val = named_values[name];
  if (val) {
    return peloton::codegen::Value(
        peloton::codegen::type::Type(type::TypeId::DECIMAL, false), val);
  }
  return LogErrorV("Unknown variable name");
}

// Codegen for BinaryExprAST
peloton::codegen::Value BinaryExprAST::Codegen(
    peloton::codegen::CodeGen &codegen) {
  std::cout << "Binary ExprAST codegen\n";

  peloton::codegen::Value left = lhs->Codegen(codegen);
  peloton::codegen::Value right = rhs->Codegen(codegen);
  if (left.GetValue() == 0 || right.GetValue() == 0) return peloton::codegen::Value();

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
    peloton::codegen::CodeGen &codegen) {
  std::cout << "CallExprAST codegen\n";

  // Get llvm::function ptr from name
  auto *callee_func = codegen.LookupPlpgsqlUDF(callee);
  if (callee_func == 0) {
    return LogErrorV("Unknown function referenced");
  }

  // If argument mismatch error.
  if (callee_func->arg_size() != args.size())
    return LogErrorV("Incorrect # arguments passed");

  std::vector<llvm::Value *> args_val;
  for (unsigned i = 0, size = args.size(); i != size; ++i) {
    args_val.push_back(args[i]->Codegen(codegen).GetValue());
    if (args_val.back() == 0) {
      return LogErrorV("Arguments could not be passed in");
    }
  }

  auto *call_ret = codegen.CallFunc(callee_func, args_val);

  auto call_val = peloton::codegen::Value(
      peloton::codegen::type::Type(type::TypeId::DECIMAL, false), call_ret);

  return call_val;
}

// Codegen for PrototypeAST
peloton::codegen::Value PrototypeAST::Codegen(
    peloton::codegen::CodeGen &codegen) {
  std::cout << "PrototypeAST codegen\n";
  // TODO(PP) : Later change this to handle any return type and any argument
  // type
  std::vector<llvm::Type *> arg_types(args.size(), codegen.DoubleType());
  auto ret_type = codegen.DoubleType();
  auto *func_type = llvm::FunctionType::get(ret_type, arg_types, false);

  auto *func_ptr = codegen.RegisterPlpgsqlUDF(name, func_type);

  // Set names for all arguments.
  unsigned idx = 0;
  for (auto iter = func_ptr->arg_begin(), end = func_ptr->arg_end();
       iter != end; iter++) {
    iter->setName(args[idx]);
    named_values[args[idx]] = iter;
  }

  auto proto_val = peloton::codegen::Value(
      peloton::codegen::type::Type(type::TypeId::INVALID, false), func_ptr);

  return proto_val;
}

// Codegen for FunctionAST
peloton::codegen::Value FunctionAST::Codegen(
    peloton::codegen::CodeGen &codegen) {
  std::cout << "FunctionAST codegen\n";
  // First, check for an existing function from a previous 'extern' declaration.
  named_values.clear();

  peloton::codegen::Value proto_val = proto->Codegen(codegen);
  llvm::Function *func_ptr =
      reinterpret_cast<llvm::Function *>(proto_val.GetValue());

  if (func_ptr == 0) return LogErrorV("FunctionAST: Unable to codegen proto");

  auto &code_context_ = codegen.GetCodeContext();

  // Set the entry point of the function
  llvm::BasicBlock *entry_bb =
      llvm::BasicBlock::Create(code_context_.GetContext(), "entry", func_ptr);
  code_context_.GetBuilder().SetInsertPoint(entry_bb);

  peloton::codegen::Value ret = body->Codegen(codegen);

  if (ret.GetValue()) {
    code_context_.GetBuilder().CreateRet(ret.GetValue());

    auto func_ast_val = peloton::codegen::Value(
        peloton::codegen::type::Type(type::TypeId::INVALID, false), func_ptr);

    return func_ast_val;
  }

  return peloton::codegen::Value();
}

std::unique_ptr<ExprAST> LogError(const char *str) {
  fprintf(stderr, "Error: %s\n", str);
  return 0;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *str) {
  LogError(str);
  return 0;
}
FunctionAST *ErrorF(const char *str) {
  LogError(str);
  return 0;
}

peloton::codegen::Value LogErrorV(const char *str) {
  LogError(str);
  return peloton::codegen::Value();
}

}  // namespace udf
}  // namespace peloton
