#include "mytoy/Dialect.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/OpImplementation.h"
#include "mlir/Support/LLVM.h"
#include "mlir/Interfaces/FunctionImplementation.h"

/// Dialect registration
#include "mytoy/Dialect.cpp.inc"

namespace mlir::mytoy {

/// Dialect initialization
void MyToyDialect::initialize() {
  addOperations<
#define GET_OP_LIST
#include "mytoy/Ops.cpp.inc"
      >();
}

} // namespace mlir::mytoy

/// Ops definitions
#define GET_OP_CLASSES
#include "mytoy/Ops.cpp.inc"

//===----------------------------------------------------------------------===//
// MyToy Operations
//===----------------------------------------------------------------------===//

namespace mlir {

static mlir::ParseResult parseBinaryOp(mlir::OpAsmParser &parser,
                                       mlir::OperationState &result) {
  SmallVector<mlir::OpAsmParser::UnresolvedOperand, 2> operands;
  SMLoc operandsLoc = parser.getCurrentLocation();
  Type type;
  if (parser.parseOperandList(operands, /*requiredOperandCount=*/2) ||
      parser.parseOptionalAttrDict(result.attributes) ||
      parser.parseColonType(type))
    return mlir::failure();

  // If the type is a function type, it contains the input and result types of
  // this operation.
  if (FunctionType funcType = llvm::dyn_cast<FunctionType>(type)) {
    if (parser.resolveOperands(operands, funcType.getInputs(), operandsLoc,
                               result.operands))
      return mlir::failure();
    result.addTypes(funcType.getResults());
    return mlir::success();
  }

  // Otherwise, the parsed type is the type of both operands and results.
  if (parser.resolveOperands(operands, type, result.operands))
    return mlir::failure();
  result.addTypes(type);
  return mlir::success();
}

static void printBinaryOp(mlir::OpAsmPrinter &printer, mlir::Operation *op) {
  printer << " " << op->getOperands();
  printer.printOptionalAttrDict(op->getAttrs());
  printer << " : ";

  // If all of the types are the same, print the type directly.
  Type resultType = *op->result_type_begin();
  if (llvm::all_of(op->getOperandTypes(),
                   [=](Type type) { return type == resultType; })) {
    printer << resultType;
    return;
  }

  // Otherwise, print a functional type.
  printer.printFunctionalType(op->getOperandTypes(), op->getResultTypes());
}

} // namespace mlir

//===----------------------------------------------------------------------===//
// ConstantOp
//===----------------------------------------------------------------------===//

namespace mlir::mytoy {

void ConstantOp::build(::mlir::OpBuilder &odsBuilder,
                       ::mlir::OperationState &odsState, double value) {
  auto resultType0 = RankedTensorType::get({}, odsBuilder.getF64Type());
  auto valueAttribute = DenseElementsAttr::get(resultType0, value);
  ConstantOp::build(odsBuilder, odsState, resultType0, valueAttribute);
}

void ConstantOp::build(::mlir::OpBuilder &odsBuilder,
                       ::mlir::OperationState &odsState,
                       DenseElementsAttr value) {
  auto resultType0 = RankedTensorType::get({}, odsBuilder.getF64Type());
  assert(value.getType() == resultType0 &&
         "value must have the same type as the result type");
  ConstantOp::build(odsBuilder, odsState, resultType0, value);
}

::llvm::LogicalResult ConstantOp::verify() {
  // If the return type of the constant is not an unranked tensor, the shape
  // must match the shape of the attribute holding the data.
  auto resultType =
      llvm::dyn_cast<mlir::RankedTensorType>(getResult().getType());
  if (!resultType)
    return success();

  // Check that the rank of the attribute type matches the rank of the constant
  // result type.
  auto attrType = llvm::cast<mlir::RankedTensorType>(getValue().getType());
  if (attrType.getRank() != resultType.getRank()) {
    return emitOpError("return type must match the one of the attached value "
                       "attribute: ")
           << attrType.getRank() << " != " << resultType.getRank();
  }

  // Check that each of the dimensions match between the two types.
  for (int dim = 0, dimE = attrType.getRank(); dim < dimE; ++dim) {
    if (attrType.getShape()[dim] != resultType.getShape()[dim]) {
      return emitOpError(
                 "return type shape mismatches its attribute at dimension ")
             << dim << ": " << attrType.getShape()[dim]
             << " != " << resultType.getShape()[dim];
    }
  }
  return mlir::success();
}

::mlir::ParseResult ConstantOp::parse(::mlir::OpAsmParser &parser,
                                      ::mlir::OperationState &result) {
  mlir::DenseElementsAttr value;
  if (parser.parseOptionalAttrDict(result.attributes) ||
      parser.parseAttribute(value, "value", result.attributes))
    return failure();

  result.addTypes(value.getType());
  return success();
}

void ConstantOp::print(::mlir::OpAsmPrinter &p) {
  p << " ";
  p.printOptionalAttrDict((*this)->getAttrs(), /*elidedAttrs=*/{"value"});
  p << getValue();
}

} // namespace mlir::mytoy

//===----------------------------------------------------------------------===//
// AddOp
//===----------------------------------------------------------------------===//

namespace mlir::mytoy {

void AddOp::build(::mlir::OpBuilder &odsBuilder,
                  ::mlir::OperationState &odsState, Value lhs, Value rhs) {
  ::mlir::Type resultType0 = UnrankedTensorType::get(odsBuilder.getF64Type());
  AddOp::build(odsBuilder, odsState, resultType0, lhs, rhs);
}

::mlir::ParseResult AddOp::parse(::mlir::OpAsmParser &parser,
                                 ::mlir::OperationState &result) {
  return parseBinaryOp(parser, result);
}

void AddOp::print(::mlir::OpAsmPrinter &p) { return printBinaryOp(p, *this); }

} // namespace mlir::mytoy

namespace mlir::mytoy {

void FuncOp::build(::mlir::OpBuilder &odsBuilder,
                   ::mlir::OperationState &odsState, StringRef name,
                   FunctionType type, ArrayRef<NamedAttribute> attrs) {

  buildWithEntryBlock(odsBuilder, odsState, name, type, attrs,
                      type.getInputs());
}

mlir::ParseResult FuncOp::parse(mlir::OpAsmParser &parser,
                                mlir::OperationState &result) {
  // Dispatch to the FunctionOpInterface provided utility method that parses the
  // function operation.
  auto buildFuncType =
      [](mlir::Builder &builder, llvm::ArrayRef<mlir::Type> argTypes,
         llvm::ArrayRef<mlir::Type> results,
         mlir::function_interface_impl::VariadicFlag,
         std::string &) { return builder.getFunctionType(argTypes, results); };

  return mlir::function_interface_impl::parseFunctionOp(
      parser, result, /*allowVariadic=*/false,
      getFunctionTypeAttrName(result.name), buildFuncType,
      getArgAttrsAttrName(result.name), getResAttrsAttrName(result.name));
}

void FuncOp::print(mlir::OpAsmPrinter &p) {
  // Dispatch to the FunctionOpInterface provided utility method that prints the
  // function operation.
  mlir::function_interface_impl::printFunctionOp(
      p, *this, /*isVariadic=*/false, getFunctionTypeAttrName(),
      getArgAttrsAttrName(), getResAttrsAttrName());
}

} // namespace mlir::mytoy
