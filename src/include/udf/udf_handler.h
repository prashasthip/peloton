#include "type/value.h"
#include "udf/udf_parser.h"

namespace peloton {

namespace concurrency {
class Transaction;
}

namespace udf {

class UDFHandler {
 public:
  UDFHandler();

  peloton::codegen::CodeContext& Execute(concurrency::Transaction* txn,
                                         std::string func_name,
                                         std::string func_body,
                                         std::vector<std::string> args_name,
                                         std::vector<arg_type> args_type,
                                         arg_type ret_type);
};

extern UDFHandler g_udf_handler;

}  // namespace udf
}  // namespace peloton