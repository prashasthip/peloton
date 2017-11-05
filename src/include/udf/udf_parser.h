#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>  // Remove later
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "type/type.h"
#include "udf/ast_nodes.h"

namespace peloton {

namespace concurrency {
class Transaction;
}

namespace udf {

using arg_type = type::TypeId;

class UDFParser {
 public:

  UDFParser(UNUSED_ATTRIBUTE concurrency::Transaction* txn);

  codegen::CodeContext& Compile(std::string func_name, std::string func_body,
            std::vector<std::string> args_name, std::vector<arg_type> args_type,
            arg_type ret_type);

 private:
  std::string identifier_str_;  // Filled in if tok_identifier
  int num_val_;              // Filled in if tok_number
  std::string func_body_string_;
  int cur_tok_;
  int last_char_;

  std::map<char, int> binop_precedence_;
  std::string::iterator func_body_iterator_;

  enum token {
    tok_eof = -1,

    // commands
    tok_def = -2,     // Use till code is merged
    tok_extern = -3,  // Not using

    // primary
    tok_identifier = -4,
    tok_number = -5,

    // Miscellaneous
    tok_return = -6,
    tok_begin = -7,
    tok_end = -8,
    tok_semicolon = -9,
    tok_comma = -10
  };

  // Helper routines
  int GetNextChar();
  int PeekNext();
  int GetTok();
  int GetNextToken();
  int GetTokPrecedence();

  // Parsing Functions
  std::unique_ptr<ExprAST> ParseNumberExpr();
  std::unique_ptr<ExprAST> ParseParenExpr();
  std::unique_ptr<ExprAST> ParseIdentifierExpr();
  std::unique_ptr<ExprAST> ParseReturn();
  std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                         std::unique_ptr<ExprAST> LHS);
  std::unique_ptr<ExprAST> ParseExpression();
  //std::unique_ptr<PrototypeAST> ParsePrototype();
  std::unique_ptr<FunctionAST> ParseDefinition();
  std::unique_ptr<ExprAST> ParsePrimary();

  llvm::Type *GetCodegenParamType(arg_type type_val,
    peloton::codegen::CodeGen &cg);
};

}  // namespace udf
}  // namespace peloton