#include "udf/udf_parser.h"

namespace peloton {
namespace udf {

using namespace std;

UDFParser::UDFParser(UNUSED_ATTRIBUTE concurrency::Transaction *txn,
                     std::string func_name, std::string func_body,
                     std::vector<std::string> args_name,
                     std::vector<arg_type> args_type, arg_type ret_type)
    : name_{func_name},
      body_{func_body},
      args_name_{args_name},
      args_type_{args_type},
      ret_type_{ret_type} {
  // Install the binOp priorities
  BinopPrecedence['<'] = 10;
  BinopPrecedence['>'] = 10;
  BinopPrecedence['+'] = 20;
  BinopPrecedence['-'] = 20;
  BinopPrecedence['/'] = 40;
  BinopPrecedence['*'] = 40;  // highest.
}

codegen::CodeContext &UDFParser::Compile() {
  std::cout << "Inside compile\n";
  std::cout << "Function body : " << body_ << "\n";

  // To contain the context of the UDF
  codegen::CodeContext *code_context = new codegen::CodeContext();
  codegen::CodeGen cg{*code_context};

  func_body_string = body_;
  func_body_iterator = func_body_string.begin();
  LastChar = ' ';
  std::cout << "Peeked" << peekNext() << "\n";
  if (auto F = ParseDefinition()) {
    if (auto *func_ptr = F->codegen(cg).GetValue()) {
      fprintf(stderr, "Read function definition");
      func_ptr->dump();
      return cg.GetCodeContext();
    }
  }
  fprintf(stderr, "Err parsing the function");
  // Error in function body
  return cg.GetCodeContext();
  ;
}

int UDFParser::getNextChar() {
  int ret = -1;
  if (func_body_iterator != func_body_string.end()) {
    ret = (int)(*func_body_iterator);
  }
  func_body_iterator++;
  return ret;
}

int UDFParser::peekNext() {
  int ret = -1;
  if (func_body_iterator + 1 != func_body_string.end()) {
    ret = (int)(*(func_body_iterator + 1));
  }
  return ret;
}

int UDFParser::GetTokPrecedence() {
  if (!isascii(CurTok)) return -1;

  // Make sure it's a declared binop.
  int TokPrec = BinopPrecedence[CurTok];
  if (TokPrec <= 0) return -1;
  return TokPrec;
}

int UDFParser::gettok() {
  cout << "LAstChar " << LastChar << "\n";

  // Skip any whitespace.
  while (isspace(LastChar)) LastChar = getNextChar();

  cout << "1 LAstChar " << LastChar << "\n";

  if (isalpha(LastChar)) {  // identifier: [a-zA-Z][a-zA-Z0-9]*
    IdentifierStr = LastChar;
    while (isalnum((LastChar = getNextChar()))) IdentifierStr += LastChar;
    if (IdentifierStr == "def")  // Remove this later
      return tok_def;
    if (IdentifierStr == "BEGIN" || IdentifierStr == "begin") return tok_begin;
    if (IdentifierStr == "END" || IdentifierStr == "end") return tok_end;
    if (IdentifierStr == "RETURN" || IdentifierStr == "return")
      return tok_return;
    return tok_identifier;
  }

  /*
  TODO @prashasp: Do better error-handling for digits
  */
  if (isdigit(LastChar) || LastChar == '.') {  // Number: [0-9.]+
    std::string NumStr;
    do {
      NumStr += LastChar;
      LastChar = getNextChar();
    } while (isdigit(LastChar) || LastChar == '.');

    NumVal = strtod(NumStr.c_str(), nullptr);
    return tok_number;
  }

  // Semicolon
  if (LastChar == ';') {
    LastChar = getNextChar();
    return tok_semicolon;
  }

  if (LastChar == ',') {
    LastChar = getNextChar();
    return tok_comma;
  }

  // Handles single line comments
  // (May not enter, since everything is flattened into one line)
  if (LastChar == '-') {
    int nextChar = peekNext();
    if (nextChar == '-') {
      LastChar = getNextChar();
      // Comment until end of line.
      do
        LastChar = getNextChar();
      while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

      if (LastChar != EOF) return gettok();
    }
  }

  // Check for end of file.  Don't eat the EOF.
  if (LastChar == EOF) return tok_eof;

  // Otherwise, just return the character as its ascii value.
  int ThisChar = LastChar;
  LastChar = getNextChar();
  return ThisChar;
}

int UDFParser::getNextToken() { return CurTok = gettok(); }

std::unique_ptr<ExprAST> UDFParser::ParseNumberExpr() {
  auto Result = llvm::make_unique<NumberExprAST>(NumVal);
  getNextToken();  // consume the number

  if (CurTok == tok_semicolon) {
    getNextToken();
  }
  return std::move(Result);
}

/// parenexpr ::= '(' expression ')'
std::unique_ptr<ExprAST> UDFParser::ParseParenExpr() {
  getNextToken();  // eat (.
  auto V = ParseExpression();
  if (!V) return nullptr;

  if (CurTok != ')') return LogError("expected ')'");
  getNextToken();  // eat ).

  if (CurTok == tok_semicolon) {
    getNextToken();
  }

  return V;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
std::unique_ptr<ExprAST> UDFParser::ParseIdentifierExpr() {
  std::string IdName = IdentifierStr;

  getNextToken();  // eat identifier.

  if (CurTok == tok_semicolon) {  // Simple variable ref.
    getNextToken();
    return llvm::make_unique<VariableExprAST>(IdName);
  }

  // Has to be a func Call.
  // if(CurTok == tok_comma || CurTok == ')') {
  if (CurTok != '(') {
    return llvm::make_unique<VariableExprAST>(IdName);
  }

  // Enters in case nextChar is '(' so it is a function call

  getNextToken();  // eat (
  std::vector<unique_ptr<ExprAST>> Args;
  if (CurTok != ')') {
    while (true) {
      auto Arg = ParseExpression();
      if (Arg) {
        Args.push_back(std::move(Arg));
      } else {
        return nullptr;
      }

      if (CurTok == ')') {
        std::cout << "exiting\n";
        break;
      }

      if (CurTok != tok_comma) {
        return LogError("Expected ')' or ',' in argument list");
      }
      getNextToken();
    }
  }
  // Eat the ')'.
  getNextToken();

  if (CurTok == tok_semicolon) {
    // Eat the ';''
    getNextToken();
  }

  return llvm::make_unique<CallExprAST>(IdName, std::move(Args));
}

std::unique_ptr<ExprAST> UDFParser::ParseReturn() {
  getNextToken();  // eat the return
  return ParsePrimary();
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
std::unique_ptr<ExprAST> UDFParser::ParsePrimary() {
  std::cout << "Inside parse PRimary\n";
  switch (CurTok) {
    default:
      std::cout << "Unknown tok " << CurTok << "\n";
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
    int ExprPrec, std::unique_ptr<ExprAST> LHS) {
  // If this is a binop, find its precedence.
  while (true) {
    int TokPrec = GetTokPrecedence();

    // If this is a binop that binds at least as tightly as the current binop,
    // consume it, otherwise we are done.
    if (TokPrec < ExprPrec) return LHS;

    // Okay, we know this is a binop.
    int BinOp = CurTok;
    std::cout << "BINOP " << BinOp << "\n";
    getNextToken();  // eat binop

    // Parse the primary expression after the binary operator.
    auto RHS = ParsePrimary();
    if (!RHS) return nullptr;

    // If BinOp binds less tightly with RHS than the operator after RHS, let
    // the pending operator take RHS as its LHS.
    int NextPrec = GetTokPrecedence();
    if (TokPrec < NextPrec) {
      RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
      if (!RHS) return nullptr;
    }

    // Merge LHS/RHS.
    LHS =
        llvm::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
  }
}

/// expression
///   ::= primary binoprhs
///
std::unique_ptr<ExprAST> UDFParser::ParseExpression() {
  std::cout << "Inside parseExpression\n";
  auto LHS = ParsePrimary();
  if (!LHS) return nullptr;

  return ParseBinOpRHS(0, std::move(LHS));
}

/// prototype
///   ::= id '(' id* ')'
std::unique_ptr<PrototypeAST> UDFParser::ParsePrototype() {
  std::cout << "inside PArse prototype\n";
  std::cout << "Successfully parsed fn def\n";
  return llvm::make_unique<PrototypeAST>(name_, std::move(args_name_));
}

/// definition ::= 'def' prototype expression
std::unique_ptr<FunctionAST> UDFParser::ParseDefinition() {
  getNextToken();  // eat begin.
  std::cout << CurTok << "Curtok after eating begin 1\n";
  getNextToken();
  std::cout << CurTok << "Curtok after eating begin 2\n";
  std::cout << "inside parsedef\n";
  auto Proto = ParsePrototype();
  if (!Proto) return nullptr;

  if (auto E = ParseExpression())
    return llvm::make_unique<FunctionAST>(std::move(Proto), std::move(E));
  return nullptr;
}

}  // namespace udf
}  // namespace peloton