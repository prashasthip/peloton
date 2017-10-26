#include "udf/udf_handler.h"

namespace peloton {
namespace udf {

UDFHandler::UDFHandler() {}

peloton::codegen::CodeContext& UDFHandler::Execute(UNUSED_ATTRIBUTE concurrency::Transaction *txn,
		std::string func_name, std::string func_body,
		UNUSED_ATTRIBUTE std::vector<std::string> args_name, 
		UNUSED_ATTRIBUTE std::vector<arg_type> args_type,
		UNUSED_ATTRIBUTE arg_type ret_type) {

	std::unique_ptr<UDFParser> parser(new UDFParser(txn, func_name, func_body,
							args_name, args_type, ret_type)); 

	std::cout << "Done creating parser object\n";
	// Get the Codegened Function Pointer
	peloton::codegen::CodeContext& code_context= parser->Compile();
	return code_context;
}

UDFHandler g_udf_handler;

}
}
