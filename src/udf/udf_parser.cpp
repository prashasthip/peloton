#include "udf/udf_parser.h"

namespace peloton {
namespace udf {

using namespace std;

UDFParser::UDFParser(UNUSED_ATTRIBUTE concurrency::Transaction *txn) {
  // Install the binOp priorities
  binop_precedence_['<'] = 10;
  binop_precedence_['>'] = 10;
  binop_precedence_['+'] = 20;
  binop_precedence_['-'] = 20;
  binop_precedence_['/'] = 40;
  binop_precedence_['*'] = 40;  // highest.
}

codegen::CodeContext &UDFParser::Compile(std::string func_name,
    std::string func_body,
    std::vector<std::string> args_name,
    std::vector<arg_type> args_type, arg_type ret_type) {

  std::cout << "Inside compile\n";
  std::cout << "Function body : " << func_body << "\n";

  // To contain the context of the UDF
  codegen::CodeContext *code_context = new codegen::CodeContext();
  codegen::CodeGen cg{*code_context};

  llvm::Type *llvm_ret_type = GetCodegenParamType(ret_type, cg);

  // vector of pair of <argument name, argument type>
  std::vector<std::pair<std::string, llvm::Type *>> llvm_args;

  auto iterator_arg_name = args_name.begin();
  auto iterator_arg_type = args_type.begin();

  while(iterator_arg_name != args_name.end() &&
        iterator_arg_type != args_type.end()) {
    llvm_args.emplace_back(*iterator_arg_name,
        GetCodegenParamType(*iterator_arg_type, cg));

    ++iterator_arg_name;
    ++iterator_arg_type;
  }

  // Construct the Function Builder object
  codegen::FunctionBuilder fb{*code_context, func_name,
      llvm_ret_type, llvm_args};

  func_body_string_ = func_body;
  func_body_iterator_ = func_body_string_.begin();
  last_char_ = ' ';
  std::cout << "Peeked" << PeekNext() << "\n";
  if (auto func = ParseDefinition()) {
    if (auto *func_ptr = func->Codegen(cg, fb)) {
      fprintf(stderr, "Read function definition");
      func_ptr->dump();
    }
  }
  return cg.GetCodeContext();
}

llvm::Type *UDFParser::GetCodegenParamType(arg_type type_val,
    peloton::codegen::CodeGen &cg) {
  // TODO(PP) : Add more types later
  // For now I am assuming only doubles as parameters
  if(type_val == type::TypeId::INTEGER) {
    return cg.Int32Type();
  } else if(type_val == type::TypeId::DECIMAL) {
    return cg.DoubleType();
  } else {
  //For now assume it to be a bool to keep compiler happy
    return cg.BoolType();
  }
}

int UDFParser::GetNextChar() {
  int ret = -1;
  if (func_body_iterator_ != func_body_string_.end()) {
    ret = (int)(*func_body_iterator_);
  }
  func_body_iterator_++;
  return ret;
}

int UDFParser::PeekNext() {
  int ret = -1;
  if (func_body_iterator_ + 1 != func_body_string_.end()) {
    ret = (int)(*(func_body_iterator_ + 1));
  }
  return ret;
}

int UDFParser::GetTokPrecedence() {
  if (!isascii(cur_tok_)) {
    return -1;
  }

  // Make sure it's a declared binop.
  int tok_prec = binop_precedence_[cur_tok_];
  if (tok_prec <= 0) {
    return -1;
  }
  return tok_prec;
}

int UDFParser::GetTok() {
  cout << "last_char_ " << last_char_ << "\n";

  // Skip any whitespace.
  while (isspace(last_char_)) {
    last_char_ = GetNextChar();
  }

  cout << "1 last_char_ " << last_char_ << "\n";

  if (isalpha(last_char_)) {  // identifier: [a-zA-Z][a-zA-Z0-9]*
    identifier_str_ = last_char_;

    while (isalnum((last_char_ = GetNextChar()))) {
      identifier_str_ += last_char_;
    }

    if (identifier_str_ == "BEGIN" || identifier_str_ == "begin") {
     return tok_begin;
    }
    if (identifier_str_ == "END" || identifier_str_ == "end") {
     return tok_end;
    }
    if (identifier_str_ == "RETURN" || identifier_str_ == "return") {
      return tok_return;
    }
    return tok_identifier;
  }

  /*
  TODO @prashasp: Do better error-handling for digits
  */
  if (isdigit(last_char_) || last_char_ == '.') {  // Number: [0-9.]+
    std::string num_str;
    do {
      num_str += last_char_;
      last_char_ = GetNextChar();
    } while (isdigit(last_char_) || last_char_ == '.');

    num_val_ = strtod(num_str.c_str(), nullptr);
    return tok_number;
  }

  // Semicolon
  if (last_char_ == ';') {
    last_char_ = GetNextChar();
    return tok_semicolon;
  }

  if (last_char_ == ',') {
    last_char_ = GetNextChar();
    return tok_comma;
  }

  // Handles single line comments
  // (May not enter, since everything is flattened into one line)
  if (last_char_ == '-') {
    int nextChar = PeekNext();
    if (nextChar == '-') {
      last_char_ = GetNextChar();
      // Comment until end of line.
      do {
        last_char_ = GetNextChar();
      } while (last_char_ != EOF && last_char_ != '\n' && last_char_ != '\r');

      if (last_char_ != EOF) {
       return GetTok();
      }
    }
  }

  // Check for end of file.  Don't eat the EOF.
  if (last_char_ == EOF) {
    return tok_eof;
  }

  // Otherwise, just return the character as its ascii value.
  int this_char = last_char_;
  last_char_ = GetNextChar();
  return this_char;
}

int UDFParser::GetNextToken() {
 return cur_tok_ = GetTok();
}

std::unique_ptr<ExprAST> UDFParser::ParseNumberExpr() {
  auto result = llvm::make_unique<NumberExprAST>(num_val_);
  GetNextToken();  // consume the number

  if (cur_tok_ == tok_semicolon) {
    GetNextToken();
  }
  return std::move(result);
}

/// parenexpr ::= '(' expression ')'
std::unique_ptr<ExprAST> UDFParser::ParseParenExpr() {
  GetNextToken();  // eat (.
  auto expr = ParseExpression();
  if (!expr) {
    return nullptr;
  }

  if (cur_tok_ != ')') {
   return LogError("expected ')'");
  }

  GetNextToken();  // eat ).

  if (cur_tok_ == tok_semicolon) {
    GetNextToken();
  }

  return expr;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
std::unique_ptr<ExprAST> UDFParser::ParseIdentifierExpr() {
  std::string id_name = identifier_str_;

  GetNextToken();  // eat identifier.

  if (cur_tok_ == tok_semicolon) {  // Simple variable ref.
    GetNextToken();
    return llvm::make_unique<VariableExprAST>(id_name);
  }

  // Has to be a func Call.
  // if(cur_tok_ == tok_comma || cur_tok_ == ')') {
  if (cur_tok_ != '(') {
    return llvm::make_unique<VariableExprAST>(id_name);
  }

  // Enters in case nextChar is '(' so it is a function call

  GetNextToken();  // eat (
  std::vector<unique_ptr<ExprAST>> args;
  if (cur_tok_ != ')') {
    while (true) {
      auto arg = ParseExpression();
      if (arg) {
        args.push_back(std::move(arg));
      } else {
        return nullptr;
      }

      if (cur_tok_ == ')') {
        std::cout << "exiting\n";
        break;
      }

      if (cur_tok_ != tok_comma) {
        return LogError("Expected ')' or ',' in argument list");
      }
      GetNextToken();
    }
  }
  // Eat the ')'.
  GetNextToken();

  if (cur_tok_ == tok_semicolon) {
    // Eat the ';''
    GetNextToken();
  }

  return llvm::make_unique<CallExprAST>(id_name, std::move(args));
}

std::unique_ptr<ExprAST> UDFParser::ParseReturn() {
  GetNextToken();  // eat the return
  return ParsePrimary();
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
std::unique_ptr<ExprAST> UDFParser::ParsePrimary() {
  std::cout << "Inside Parse Primary\n";
  switch (cur_tok_) {
    default:
      std::cout << "Unknown tok " << cur_tok_ << "\n";
      return LogError("unknown token when expecting an expression");
    case tok_identifier:
      std::cout << "Got a tok_identifier\n";
      return ParseIdentifierExpr();
    case tok_number:
      std::cout << "Got a tok_number\n";
      return ParseNumberExpr();
    case '(':
      std::cout << "Got a (\n";
      return ParseParenExpr();
    case tok_return:
      std::cout << "Got a tok_return\n";
      return ParseReturn();
    case tok_semicolon:
      return nullptr;
  }
}

/// binoprhs
///   ::= ('+' primary)*
std::unique_ptr<ExprAST> UDFParser::ParseBinOpRHS(
    int expr_prec, std::unique_ptr<ExprAST> lhs) {
  // If this is a binop, find its precedence.
  while (true) {
    int tok_prec = GetTokPrecedence();

    // If this is a binop that binds at least as tightly as the current binop,
    // consume it, otherwise we are done.
    if (tok_prec < expr_prec) {
     return lhs;
    }

    // Okay, we know this is a binop.
    int bin_op = cur_tok_;
    std::cout << "BINOP " << bin_op << "\n";
    GetNextToken();  // eat binop

    // Parse the primary expression after the binary operator.
    auto rhs = ParsePrimary();
    if (!rhs) {
     return nullptr;
    }

    // If BinOp binds less tightly with rhs than the operator after rhs, let
    // the pending operator take rhs as its lhs.
    int next_prec = GetTokPrecedence();
    if (tok_prec < next_prec) {
      rhs = ParseBinOpRHS(tok_prec + 1, std::move(rhs));
      if (!rhs) {
       return nullptr;
      }
    }

    // Merge lhs/rhs.
    lhs = llvm::make_unique<BinaryExprAST>(bin_op, std::move(lhs),
            std::move(rhs));
  }
}

/// expression
///   ::= primary binoprhs
///
std::unique_ptr<ExprAST> UDFParser::ParseExpression() {
  std::cout << "Inside ParseExpression\n";
  auto lhs = ParsePrimary();
  if (!lhs) {
    return nullptr;
  }

  return ParseBinOpRHS(0, std::move(lhs));
}

/// definition ::= 'def' prototype expression
std::unique_ptr<FunctionAST> UDFParser::ParseDefinition() {
  GetNextToken();  // eat begin.
  std::cout << cur_tok_ << "cur_tok_ after eating begin 1\n";
  GetNextToken();
  std::cout << cur_tok_ << "cur_tok_ after eating begin 2\n";
  std::cout << "inside Parsedef\n";
  //auto proto = ParsePrototype();

  //if (!proto) return nullptr;

  if (auto expr = ParseExpression()) {
    return llvm::make_unique<FunctionAST>(std::move(expr));
  }
  return nullptr;
}

}  // namespace udf
}  // namespace peloton