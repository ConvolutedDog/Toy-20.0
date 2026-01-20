#include "mlir/Parser/Parser.h"
#include "mytoy/AST.h"
#include "mytoy/Dialect.h"
#include "mytoy/MLIRGen.h"
#include "mytoy/Parser.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorOr.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include <iostream>
#include <memory>
#include <string>

namespace cl = llvm::cl;

static cl::opt<std::string> inputFilename(cl::Positional,
                                          cl::desc("<input toy file>"),
                                          cl::init("-"),
                                          cl::value_desc("filename"));
namespace {
enum class Emit : uint8_t { AST, MLIR };
} // namespace

static cl::opt<Emit>
    emit(cl::desc("Choose output:"),
         cl::values(clEnumValN(Emit::AST, "ast", "Emit AST"),
                    clEnumValN(Emit::MLIR, "mlir", "Emit MLIR")),
         cl::init(Emit::AST));

namespace {
enum InputType { Toy, MLIR };
} // namespace

static cl::opt<enum InputType> inputType(
    "x", cl::init(Toy), cl::desc("Decided the kind of output desired"),
    cl::values(clEnumValN(Toy, "toy", "load the input file as a Toy source.")),
    cl::values(clEnumValN(MLIR, "mlir",
                          "load the input file as an MLIR file")));

namespace {
llvm::StringRef readInputFile(const std::string &path) {
  llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> fileOrErr =
      llvm::MemoryBuffer::getFileOrSTDIN(path);
  if (std::error_code ec = fileOrErr.getError()) {
    llvm::errs() << "Could not open input file: " << ec.message() << "\n";
    return llvm::StringRef();
  }
  llvm::StringRef buffer = fileOrErr.get()->getBuffer();
  return buffer;
}

std::unique_ptr<mytoy::ModuleAST> parseModule(const llvm::StringRef &buffer,
                                              const std::string &path) {
  if (!llvm::StringRef(path).ends_with(".toy"))
    llvm::errs() << "Input file extension not recognized, "
                 << "expected '.toy'\n";
  mytoy::Lexer lexer(buffer, path);
  mytoy::Parser parser(lexer);
  std::unique_ptr<mytoy::ModuleAST> ast = parser.parseModule();
  return std::move(ast);
}
} // namespace

static int dumpMLIR() {
  mlir::MLIRContext context;
  // Load our Dialect in this MLIR Context.
  context.getOrLoadDialect<mlir::mytoy::MyToyDialect>();

  // Handle '.toy' input to the compiler.
  if (inputType != InputType::MLIR &&
      !llvm::StringRef(inputFilename).ends_with(".mlir")) {
    llvm::StringRef buffer = readInputFile(inputFilename);
    std::unique_ptr<mytoy::ModuleAST> moduleAST =
        parseModule(buffer, inputFilename);
    if (!moduleAST)
      return 6;
    mlir::OwningOpRef<mlir::ModuleOp> module =
        mlirGen(context, *moduleAST);
    if (!module)
      return 1;

    module->dump();
    return 0;
  }

  // Otherwise, the input is '.mlir'.
  llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> fileOrErr =
      llvm::MemoryBuffer::getFileOrSTDIN(inputFilename);
  if (std::error_code ec = fileOrErr.getError()) {
    llvm::errs() << "Could not open input file: " << ec.message() << "\n";
    return -1;
  }

  // Parse the input mlir.
  llvm::SourceMgr sourceMgr;
  sourceMgr.AddNewSourceBuffer(std::move(*fileOrErr), llvm::SMLoc());
  mlir::OwningOpRef<mlir::ModuleOp> module =
      mlir::parseSourceFile<mlir::ModuleOp>(sourceMgr, &context);
  if (!module) {
    llvm::errs() << "Error can't load file " << inputFilename << "\n";
    return 3;
  }

  module->dump();
  return 0;
}

int main(int argc, char **argv) {
  cl::ParseCommandLineOptions(argc, argv, "MyToy compiler\n");

  switch (emit) {
  case Emit::AST: {
    llvm::StringRef buffer = readInputFile(inputFilename);
    std::unique_ptr<mytoy::ModuleAST> mod = parseModule(buffer, inputFilename);
    mod->dump(std::cout);
    break;
  }
  case Emit::MLIR:
    dumpMLIR();
    break;
  }

  return 0;
}
