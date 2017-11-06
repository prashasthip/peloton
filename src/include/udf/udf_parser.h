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

class UDFParser {
 public:

  UDFParser(UNUSED_ATTRIBUTE concurrency::Transaction* txn);

  void ParseUDF( codegen::CodeGen &cg, codegen::FunctionBuilder &fb,
    std::string func_body);

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
  std::unique_ptr<FunctionAST> ParseDefinition();
  std::unique_ptr<ExprAST> ParsePrimary();

};

}  // namespace udf
}  // namespace peloton