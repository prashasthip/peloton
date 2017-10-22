#include "udf/udf_parser.h"

namespace peloton {
namespace udf {

UDFParser::UDFParser(UNUSED_ATTRIBUTE concurrency::Transaction *txn, std::string func_name,
		std::string func_body,
		std::vector<std::string> args_name, 
		std::vector<arg_type> args_type, arg_type ret_type)
	:name_{func_name}, body_{func_body}, args_name_{args_name},
		args_type_{args_type}, ret_type_{ret_type}
{
	// Install the binOp priorities
	BinopPrecedence['<'] = 10;
	BinopPrecedence['>'] = 10;
	BinopPrecedence['+'] = 20;
	BinopPrecedence['-'] = 20;
	BinopPrecedence['/'] = 40;
	BinopPrecedence['*'] = 40; // highest.
}

Function *UDFParser::Compile() {
	std::cout << "Inside compile\n";
	std::cout << "Function body : " << body_ << "\n";
	func_body_string = body_;
	func_body_iterator = func_body_string.begin();
	if (FunctionAST *F = ParseDefinition()) {
	    if (Function *LF = F->Codegen()) {
	      fprintf(stderr, "Read function definition:");
	      LF->dump();
	      return LF;
	    }
	} 
	// Error in function body
	return nullptr;
}

int UDFParser::getNextChar() {
  int ret = -1;
  if(func_body_iterator != func_body_string.end()) {
    ret = (int)(*func_body_iterator);
  } 
  func_body_iterator++;
  return ret;
} 

int UDFParser::peekNext() {
  int ret = -1;
  if(func_body_iterator + 1 != func_body_string.end()) {
    ret = (int)(*(func_body_iterator + 1));
  }
  return ret;
}

int UDFParser::GetTokPrecedence() {
  if (!isascii(CurTok))
    return -1;

  // Make sure it's a declared binop.
  int TokPrec = BinopPrecedence[CurTok];
  if (TokPrec <= 0)
    return -1;
  return TokPrec;
}

int UDFParser::gettok() {
  static int LastChar = ' ';

  // Skip any whitespace.
  while (isspace(LastChar))
    LastChar = getNextChar();

  if (isalpha(LastChar)) { // identifier: [a-zA-Z][a-zA-Z0-9]*
    IdentifierStr = LastChar;
    while (isalnum((LastChar = getNextChar())))
      IdentifierStr += LastChar;
  	if(IdentifierStr == "def") // Remove this later
  		return tok_def;
    if (IdentifierStr == "BEGIN" || IdentifierStr == "begin")
      return tok_begin;
    if (IdentifierStr == "END" || IdentifierStr == "end")
      return tok_end;
    if (IdentifierStr == "RETURN" || IdentifierStr == "return")
      return tok_return;
    return tok_identifier;
  }


  /* 
  TODO @prashasp: Do better error-handling for digits 
  */
  if (isdigit(LastChar) || LastChar == '.') { // Number: [0-9.]+
    std::string NumStr;
    do {
      NumStr += LastChar;
      LastChar = getNextChar();
    } while (isdigit(LastChar) || LastChar == '.');

    NumVal = strtod(NumStr.c_str(), nullptr);
    return tok_number;
  }

  //Semicolon
  if(LastChar == ';') {
    LastChar = getNextChar();
  	return tok_semicolon;
  }

  if(LastChar == ',') {
  	LastChar = getNextChar();
  	return tok_comma;
  }

  // Handles single line comments
  // (May not enter, since everything is flattened into one line)
  if (LastChar == '-') {
    int nextChar = peekNext();
    if(nextChar == '-'){
    LastChar = getNextChar();
    // Comment until end of line.
    do
      LastChar = getNextChar();
    while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

    if (LastChar != EOF)
      return gettok();
    }
  }

  // Check for end of file.  Don't eat the EOF.
  if (LastChar == EOF)
    return tok_eof;

  // Otherwise, just return the character as its ascii value.
  int ThisChar = LastChar;
  LastChar = getNextChar();
  return ThisChar;
}

int UDFParser::getNextToken() { 
	return CurTok = gettok(); 
}

ExprAST *UDFParser::ParseNumberExpr() {
  ExprAST *Result = new NumberExprAST(NumVal);
  getNextToken(); // consume the number

  if(CurTok == tok_semicolon){
  	getNextToken();
  }
  return Result;
}

/// parenexpr ::= '(' expression ')'
ExprAST *UDFParser::ParseParenExpr() {
  getNextToken(); // eat (.
  ExprAST *V = ParseExpression();
  if (!V)
    return nullptr;

  if (CurTok != ')')
    return Error("expected ')'");
  getNextToken(); // eat ).

  if(CurTok == tok_semicolon) {
  	getNextToken();
  }

  return V;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
ExprAST *UDFParser::ParseIdentifierExpr()  {
  std::string IdName = IdentifierStr;

  getNextToken(); // eat identifier.

  if (CurTok == tok_semicolon) { // Simple variable ref.
  	getNextToken();
  	return new VariableExprAST(IdName);
  } 

  // Has to be a func Call.
  //if(CurTok == tok_comma || CurTok == ')') {
  if(CurTok != '(') {
  	return new VariableExprAST(IdName);
  }


  //Enters in case nextChar is '(' so it is a function call

  getNextToken(); // eat (
  std::vector<ExprAST*> Args;
  if (CurTok != ')') {
    while (true) {
      ExprAST *Arg = ParseExpression();
      if (Arg) {
        Args.push_back(Arg);
      } else {
      	return nullptr;
      }

      if (CurTok == ')') {
      	std::cout << "exiting\n";
      	break;
      }

      if (CurTok != tok_comma) {
      	return Error("Expected ')' or ',' in argument list");
      }
      getNextToken();
    }
  }
  // Eat the ')'.
  getNextToken();
 
  if(CurTok == tok_semicolon) {
  	// Eat the ';''
  	getNextToken();
  }

  return new CallExprAST(IdName, Args);
}


ExprAST *UDFParser::ParseReturn() {
	getNextToken(); //eat the return
	return ParsePrimary();
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
ExprAST *UDFParser::ParsePrimary() {
  std::cout <<"Inside parse PRimary\n";
  switch (CurTok) {
  default:
  	std::cout << "Unknown tok " << CurTok << "\n";
    return Error("unknown token when expecting an expression");
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
ExprAST *UDFParser::ParseBinOpRHS(int ExprPrec, ExprAST *LHS) {
  // If this is a binop, find its precedence.
  while (true) {
    int TokPrec = GetTokPrecedence();

    // If this is a binop that binds at least as tightly as the current binop,
    // consume it, otherwise we are done.
    if (TokPrec < ExprPrec)
      return LHS;

    // Okay, we know this is a binop.
    int BinOp = CurTok;
    std::cout << "BINOP " << BinOp << "\n";
    getNextToken(); // eat binop

    // Parse the primary expression after the binary operator.
    ExprAST *RHS = ParsePrimary();
    if (!RHS)
      return nullptr;

    // If BinOp binds less tightly with RHS than the operator after RHS, let
    // the pending operator take RHS as its LHS.
    int NextPrec = GetTokPrecedence();
    if (TokPrec < NextPrec) {
      RHS = ParseBinOpRHS(TokPrec + 1, RHS);
      if (!RHS)
        return nullptr;
    }

    // Merge LHS/RHS.
    LHS = new BinaryExprAST(BinOp, LHS, RHS);
  }
}

/// expression
///   ::= primary binoprhs
///
ExprAST *UDFParser::ParseExpression() {
	std::cout << "Inside parseExpression\n";
  ExprAST *LHS = ParsePrimary();
  if (!LHS)
    return nullptr;

  return ParseBinOpRHS(0, LHS);
}

/// prototype
///   ::= id '(' id* ')'
PrototypeAST *UDFParser::ParsePrototype() {
  std::cout << "inside PArse prototype\n";
  std::cout << "Successfully parsed fn def\n";
  return new PrototypeAST(name_, args_name_);
}

/// definition ::= 'def' prototype expression
FunctionAST *UDFParser::ParseDefinition() {
  getNextToken(); // eat begin.
  getNextToken();
  std::cout << CurTok << "Curtok after eating begin\n";
  std::cout << "inside parsedef\n";
  PrototypeAST *Proto = ParsePrototype();
  if (!Proto)
    return nullptr;

  if (ExprAST *E = ParseExpression())
     return new FunctionAST(Proto, E);
  return nullptr;
}



}
}