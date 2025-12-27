#pragma once

#include "mytoy/AST.h"
#include "mytoy/Lexer.h"
#include "llvm/ADT/StringRef.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <strstream>
#include <variant>
#include <vector>

namespace mytoy {

class Parser {
public:
  Parser(Lexer &lexer) : lexer(lexer) {}
  std::unique_ptr<ModuleAST> parseModule(bool testLexer = false);

private:
  std::unique_ptr<Protocol> parseProtocol();
  std::unique_ptr<VarDefExpr> parseVarDef();
  std::unique_ptr<ReturnExpr> parseReturn();
  std::unique_ptr<CallExpr> parseCall(bool hasConsumedIdentifier = false,
                                      std::string &&id = "");
  std::unique_ptr<Expr> parseLhs();
  std::unique_ptr<Expr> parseRhs();
  std::vector<double> parseSbracket();
  std::unique_ptr<Expr> parseTensor();
  std::unique_ptr<Expr> parseExpr();
  std::unique_ptr<ExprList> parseFuncBody();
  Struct parseStruct();
  Function parseFunction();

private:
  /// Record structs defined in the module.
  std::vector<Struct> structs;

private:
  /// Error reporting helper.
  void errorAt(const Location &loc, const std::string &msg);

private:
  Lexer &lexer;
};

} // namespace mytoy
