#include "mytoy/AST.h"
#include "mytoy/Parser.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorOr.h"
#include "llvm/Support/MemoryBuffer.h"
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
  mytoy::Lexer lexer(buffer, path);
  mytoy::Parser parser(lexer);
  std::unique_ptr<mytoy::ModuleAST> ast = parser.parseModule();
  return std::move(ast);
}
} // namespace

int main(int argc, char **argv) {
  cl::ParseCommandLineOptions(argc, argv, "MyToy compiler\n");

  llvm::StringRef buffer = readInputFile(inputFilename);
  std::unique_ptr<mytoy::ModuleAST> mod = parseModule(buffer, inputFilename);

  switch (emit) {
  case Emit::AST:
    mod->dump(std::cout);
    break;
  case Emit::MLIR:
    // std::cout << mlirGen(mod.get());
    break;
  }

  return 0;
}
