#include "mytoy/MLIRGen.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/Verifier.h"

#include "mlir/IR/Value.h"
#include "mytoy/Lexer.h"
#include "llvm/ADT/ScopedHashTable.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

namespace mytoy {

class MLIRGenImpl {
public:
  MLIRGenImpl(mlir::MLIRContext &context) : builder(&context) {}
  mlir::ModuleOp mlirGen(const ModuleAST &moduleAST) {
    // We create an empty MLIR module and codegen functions one at a time and
    // add them to the module.
    theModule = mlir::ModuleOp::create(builder.getUnknownLoc());

    for (const Function &f : moduleAST.getFunctions())
      mlirGen(f);

    // Verify the module after we have finished constructing it, this will check
    // the structural properties of the IR and invoke any specific verifiers we
    // have on the Toy operations.
    if (failed(mlir::verify(theModule))) {
      theModule.emitError("module verification error");
      return nullptr;
    }

    return theModule;
  }

private:
  mlir::ModuleOp theModule;
  mlir::OpBuilder builder;

  llvm::ScopedHashTable<llvm::StringRef, mlir::Value> symbolTable;

private:
  mlir::Location loc(const Location &loc) {
    return mlir::FileLineColLoc::get(builder.getStringAttr(loc.path), loc.row,
                                     loc.col);
  }

  /// Build a tensor type from a list of shape dimensions.
  mlir::Type getType(std::vector<int64_t> shape) {
    // If the shape is empty, then this type is unranked.
    if (shape.empty())
      return mlir::UnrankedTensorType::get(builder.getF64Type());

    // Otherwise, we use the given shape.
    return mlir::RankedTensorType::get(shape, builder.getF64Type());
  }

  /// Build an MLIR type from a Toy AST variable type (forward to the generic
  /// getType above).
  mlir::Type getType(const VarType &type) { return getType(type.shape); }

  mlir::mytoy::FuncOp mlirGen(const Protocol &proto) {
    /// TODO: Fix the location
    auto location = loc(NullLocation());

    // This is a generic function, the return type will be inferred later.
    // Arguments type are uniformly unranked tensors.
    llvm::SmallVector<mlir::Type, 4> argTypes(proto.getArgs().size(),
                                              getType(VarType{}));
    auto funcType = builder.getFunctionType(argTypes, {});
    return mlir::mytoy::FuncOp::create(builder, location, proto.getFunc(),
                                       funcType);
  }

  /// Declare a variable in the current scope, return success if the variable
  /// wasn't declared yet.
  llvm::LogicalResult declare(llvm::StringRef var, mlir::Value value) {
    if (symbolTable.count(var))
      return mlir::failure();
    symbolTable.insert(var, value);
    return mlir::success();
  }

  mlir::Value mlirGen(VarDefExpr &vardecl) {
    llvm::errs() << "VarDefExpr not implemented yet\n";
  }

  llvm::LogicalResult mlirGen(ReturnExpr &ret) {
    llvm::errs() << "ReturnExpr not implemented yet\n";
  }

  llvm::LogicalResult mlirGen(ExprList &blockAST) {
    llvm::ScopedHashTableScope<llvm::StringRef, mlir::Value> varScope(
        symbolTable);
    for (auto &expr : blockAST.getExprs()) {
      // Specific handling for variable declarations, return statement, and
      // print. These can only appear in block list and not in nested
      // expressions.
      if (auto *vardecl = llvm::dyn_cast<VarDefExpr>(std::remove_reference_t<VarDefExpr *>(expr.get()))) {
        if (!mlirGen(*vardecl))
          return mlir::failure();
        continue;
      }
      if (auto *ret = llvm::dyn_cast<ReturnExpr>(std::remove_reference_t<ReturnExpr *>(expr.get())))
        return mlirGen(*ret);
      /// TODO: Fix
      // if (auto *print = dyn_cast<PrintExprAST>(expr.get())) {
      //   if (mlir::failed(mlirGen(*print)))
      //     return mlir::success();
      //   continue;
      // }

      // Generic expression dispatch codegen.
      // if (!mlirGen(*expr))
      //   return mlir::failure();
    }
    return mlir::success();
  }

  mlir::mytoy::FuncOp mlirGen(const Function &funcAST) {
    // Create a scope in the symbol table to hold variable declarations.
    llvm::ScopedHashTableScope<llvm::StringRef, mlir::Value> varScope(
        symbolTable);

    // Create an MLIR function for the given prototype.
    builder.setInsertionPointToEnd(theModule.getBody());
    mlir::mytoy::FuncOp function = mlirGen(*funcAST.getProto());
    if (!function)
      return nullptr;

    // Let's start the body of the function now!
    mlir::Block &entryBlock = function.front();
    llvm::ArrayRef<std::unique_ptr<VariableExpr>> protoArgs = funcAST.getProto()->getArgs();

    // Declare all the function arguments in the symbol table.
    for (const auto nameValue :
         llvm::zip(protoArgs, entryBlock.getArguments())) {
      if (failed(declare(std::get<0>(nameValue)->getName(),
                         std::get<1>(nameValue))))
        return nullptr;
    }

    // Set the insertion point in the builder to the beginning of the function
    // body, it will be used throughout the codegen to create operations in this
    // function.
    builder.setInsertionPointToStart(&entryBlock);

    // Emit the body of the function.
    if (mlir::failed(mlirGen(*funcAST.getBody()))) {
      function.erase();
      return nullptr;
    }

    /// TODO: Fix
    // Implicitly return void if no return statement was emitted.
    // FIXME: we may fix the parser instead to always return the last expression
    // (this would possibly help the REPL case later)
    // ReturnOp returnOp;
    // if (!entryBlock.empty())
    //   returnOp = dyn_cast<ReturnOp>(entryBlock.back());
    // if (!returnOp) {
    //   ReturnOp::create(builder, loc(funcAST.getProto()->loc()));
    // } else if (returnOp.hasOperand()) {
    //   // Otherwise, if this return operation has an operand then add a result to
    //   // the function.
    //   function.setType(builder.getFunctionType(
    //       function.getFunctionType().getInputs(), getType(VarType{})));
    // }

    return function;
  }
};

mlir::OwningOpRef<mlir::ModuleOp> mlirGen(mlir::MLIRContext &context,
                                          ModuleAST &moduleAST) {
  return MLIRGenImpl(context).mlirGen(moduleAST);
}
} // namespace mytoy
