//===----------------------------------------------------------------------===//
//
//                         Peloton
//
// function_translator.h
//
// Identification: src/include/codegen/expression/function_translator.h
//
// Copyright (c) 2015-2017, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include "codegen/expression/expression_translator.h"
#include "type/type.h"

namespace peloton {

using arg_type = type::TypeId;

namespace expression {
class FunctionExpression;
}  // namespace expression

namespace codegen {

/// A translator for function expressions.
class FunctionTranslator : public ExpressionTranslator {
 public:
  FunctionTranslator(const expression::FunctionExpression &func_expr,
                     CompilationContext &context);
  llvm::Type *GetCodegenParamType(arg_type type_val,
    peloton::codegen::CodeGen &cg) const;

  codegen::Value DeriveValue(CodeGen &codegen,
                             RowBatch::Row &row) const override;
};

}  // namespace codegen
}  // namespace peloton
