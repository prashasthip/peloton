#include "udf/ast_nodes.h"
#include <iostream> // Remove later , for couts
#include "type/types.h"
#include "codegen/type/type.h"

namespace peloton {
namespace udf {

// Keeps track of symbols while declaring a function
static std::map<std::string, llvm::Value *> NamedValues;

// Codegen for NumberExprAST
peloton::codegen::Value NumberExprAST::codegen(peloton::codegen::CodeGen &codegen) {
  std::cout << "NumberExprAST codegen\n";
  auto numberCodegenVal = peloton::codegen::Value(
          peloton::codegen::type::Type(type::TypeId::DECIMAL, false),
          codegen.ConstDouble(Val)
  );
  return numberCodegenVal;
}

// Codegen for VariableExprAST
peloton::codegen::Value VariableExprAST::codegen(UNUSED_ATTRIBUTE peloton::codegen::CodeGen &codegen) {
  std::cout << "VariabelExprAST codegen\n";
  llvm::Value *val = NamedValues[Name];
  if(val) {
    return peloton::codegen::Value(
            peloton::codegen::type::Type(type::TypeId::DECIMAL, false), val);
  }
  return LogErrorV("Unknown variable name");
}

// Codegen for BinaryExprAST
peloton::codegen::Value BinaryExprAST::codegen(peloton::codegen::CodeGen &codegen) {
  std::cout << "Binary ExprAST codegen\n";

  peloton::codegen::Value L = LHS->codegen(codegen);
  peloton::codegen::Value R = RHS->codegen(codegen);
  if (L.GetValue() == 0 || R.GetValue() == 0) return peloton::codegen::Value();
  
  switch (Op) {
  case '+': 
	return L.Add(codegen, R);
  case '-':
    return L.Sub(codegen, R);
  case '*':
    return L.Mul(codegen, R);
  case '/':
    return L.Div(codegen, R);
  // TODO(PP): add < and > later on
  /*case '>':
  	L = Builder.CreateFCmpUGT(L, R, "cmpGT");
  	// Convert bool 0/1 to double 0.0 or 1.0
    return Builder.CreateUIToFP(L, Type::getDoubleTy(getGlobalContext()), "boolGT");
  case '<':
    L = Builder.CreateFCmpULT(L, R, "cmptmp");
    // Convert bool 0/1 to double 0.0 or 1.0
    return Builder.CreateUIToFP(L, Type::getDoubleTy(getGlobalContext()),
                                "boolLT"); */
  default: return LogErrorV("invalid binary operator");
  }
}

// Codegen for CallExprAST
peloton::codegen::Value CallExprAST::codegen(peloton::codegen::CodeGen &codegen) {
  std::cout << "CallExprAST codegen\n";

  // Get llvm::function ptr from name
  auto *CalleeF = codegen.LookupPlpgsqlUDF(Callee);
  if(CalleeF == 0) {
    return LogErrorV("Unknown function referenced");
  }

  // If argument mismatch error.
  if (CalleeF->arg_size() != Args.size())
    return LogErrorV("Incorrect # arguments passed");


  std::vector<llvm::Value *> ArgsV;
  for (unsigned i = 0, e = Args.size(); i != e; ++i) {
    ArgsV.push_back(Args[i]->codegen(codegen).GetValue());
    if (ArgsV.back() == 0) {
      return LogErrorV("Arguments could not be passed in");
    }
  }
  
  auto *call_ret = codegen.CallFunc(CalleeF, ArgsV);

  auto call_val = peloton::codegen::Value(
          peloton::codegen::type::Type(type::TypeId::DECIMAL, false),
          call_ret
  );

  return call_val;
}

// Codegen for PrototypeAST
peloton::codegen::Value PrototypeAST::codegen(peloton::codegen::CodeGen &codegen) {
  std::cout << "PrototypeAST codegen\n";
  // TODO(PP) : Later change this to handle any return type and any argument type
  std::vector<llvm::Type*> arg_types(Args.size(), codegen.DoubleType());
  auto ret_type = codegen.DoubleType();
  auto *FT = llvm::FunctionType::get(ret_type, arg_types, false);
  
  auto *func_ptr = codegen.RegisterPlpgsqlUDF(Name, FT);

  // Set names for all arguments.
  unsigned Idx = 0;
  for (auto iter = func_ptr->arg_begin(), end = func_ptr->arg_end(); iter != end;
       iter++) {
    iter->setName(Args[Idx]);
    NamedValues[Args[Idx]] = iter;
  }

  auto proto_val = peloton::codegen::Value(
          peloton::codegen::type::Type(type::TypeId::INVALID, false),
          func_ptr
  );

  return proto_val;
}

// Codegen for FunctionAST
peloton::codegen::Value FunctionAST::codegen(peloton::codegen::CodeGen &codegen) {
  std::cout << "FunctionAST codegen\n";
  // First, check for an existing function from a previous 'extern' declaration.
  NamedValues.clear();
  
  peloton::codegen::Value proto_val = Proto->codegen(codegen);
  llvm::Function *func_ptr = reinterpret_cast<llvm::Function *>(proto_val.GetValue());

  if (func_ptr == 0)
    return LogErrorV("FunctionAST: Unable to codegen proto");

  auto& code_context_ = codegen.GetCodeContext();

  // Set the entry point of the function
  llvm::BasicBlock *entry_bb =
      llvm::BasicBlock::Create(code_context_.GetContext(), "entry", func_ptr);
  code_context_.GetBuilder().SetInsertPoint(entry_bb);

  peloton::codegen::Value ret = Body->codegen(codegen);

  if(ret.GetValue()) {
    code_context_.GetBuilder().CreateRet(ret.GetValue());


    auto func_ast_val = peloton::codegen::Value(
            peloton::codegen::type::Type(type::TypeId::INVALID, false),
            func_ptr
    );

    return func_ast_val;
  }

  return peloton::codegen::Value();
}


std::unique_ptr<ExprAST> LogError(const char *Str) {
	fprintf(stderr, "Error: %s\n", Str);
	return 0;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
	LogError(Str);
	return 0;
}
FunctionAST *ErrorF(const char *Str) {
	LogError(Str);
	return 0;
}

peloton::codegen::Value LogErrorV(const char *Str) {
	LogError(Str);
	return peloton::codegen::Value();
}

}
}
