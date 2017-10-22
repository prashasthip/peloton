//===----------------------------------------------------------------------===//
//
//                         Peloton
//
// create_function_executor.cpp
//
// Identification: src/executor/create_function_executor.cpp
//
// Copyright (c) 2015-17, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "executor/create_function_executor.h"
#include "executor/executor_context.h"
#include "common/logger.h"
#include "catalog/catalog.h"
#include "concurrency/transaction.h"
#include "planner/create_function_plan.h"
#include "type/types.h"
#include <iostream>
#include "udf/udf_handler.h"

namespace peloton {
namespace executor {

// Constructor for create_function_executor
CreateFunctionExecutor::CreateFunctionExecutor(const planner::AbstractPlan *node,
                               ExecutorContext *executor_context)
    : AbstractExecutor(node, executor_context) {
  context = executor_context;
}

// Initialize executer
// Nothing to initialize now
bool CreateFunctionExecutor::DInit() {
  LOG_TRACE("Initializing Create Function Executer...");
  LOG_TRACE("Create Function Executer initialized!");
  return true;
}

bool CreateFunctionExecutor::DExecute() {
  LOG_TRACE("Executing Create...");
  ResultType result;
  const planner::CreateFunctionPlan &node = GetPlanNode<planner::CreateFunctionPlan>();
  auto current_txn = context->GetTransaction();
  // auto pool = context->GetPool();
  
  auto proname = node.GetFunctionName();
  //auto prolang = static_cast<oid_t>(node.GetUDFLanguage());
  //auto pronargs = node.GetNumParams();
  auto prorettype = node.GetReturnType();
  auto proargtypes = node.GetFunctionParameterTypes();
  auto proargnames = node.GetFunctionParameterNames();
  auto prosrc_bin = node.GetFunctionBody();
  //auto isReplace = node.IsReplace(); // If replace, delete from the map, also see what's to be done on the JIT side
  // For now only cater to create

  /* Pass it off to the UDF handler
	Once you get the function pointer, put that an other details into the catalog */
  
  //auto udf_h(new peloton::udf::UDFHandler());
  Function *llvm_func_ptr = peloton::udf::g_udf_handler.Execute(current_txn, proname, prosrc_bin[0], proargnames, proargtypes, prorettype);
  std::cout << "LLVM fn ptr\n";

  if(llvm_func_ptr) {
  	result = ResultType::SUCCESS;
  	// Insert into catalog
  	// If insert fails, it resets result to failure
  } else {
  	result = ResultType::FAILURE;
  }

  current_txn->SetResult(result);

   if (current_txn->GetResult() == ResultType::SUCCESS) {
      LOG_TRACE("Registered UDF successfully!");
    } else if (current_txn->GetResult() == ResultType::FAILURE) {
      LOG_TRACE("Could not register function. SAD."); 
    } else {
      LOG_TRACE("Result is: %s",
                ResultTypeToString(current_txn->GetResult()).c_str());
    }


  return false;
}
}
}
