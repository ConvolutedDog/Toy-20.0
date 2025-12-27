#include "mytoy/Parser.h"
#include "mytoy/AST.h"
#include "mytoy/Lexer.h"
#include <memory>
#include <utility>

namespace mytoy {

/// Report errors in the parser.
class BasicParserError : public std::runtime_error {
public:
  BasicParserError(const Location &loc, const std::string &msg)
      : loc(loc), msg(msg), std::runtime_error(msg) {}
  const Location &getLoc() const { return loc; }
  const std::string &getMsg() const { return msg; }
  const char *what() const noexcept final { return std::runtime_error::what(); }

private:
  Location loc;
  std::string msg;
};

class UnexceptedTokenError : public BasicParserError {
public:
  UnexceptedTokenError(const Location &loc, const std::string &msg)
      : BasicParserError(loc, msg) {}
};

std::unique_ptr<ModuleAST> Parser::parseModule(bool testLexer) {
  Token tok;
  if (testLexer) {
    while (tok != tok_eof) {
      tok = lexer.getNextToken();
      lexer.dumpCurrStat();
    }
    exit(0);
  } else {
    std::vector<Struct> structs;
    std::vector<Function> functions;
    while (tok != tok_eof) {
      tok = lexer.getNextToken();
      if (tok == tok_def) {
        Function func = parseFunction();
        functions.push_back(std::move(func));
      } else if (tok == tok_struct) {
        Struct stru = parseStruct();
        structs.push_back(std::move(stru));
      }
    }
    return std::make_unique<ModuleAST>(std::move(functions),
                                       std::move(structs));
  }
}

std::unique_ptr<Protocol> Parser::parseProtocol() {
  Location loc = lexer.getLoc();
  std::vector<std::unique_ptr<VariableExpr>> args;
  std::string func;
  if (lexer.getNextToken() != tok_identifier)
    this->errorAt(loc, "expect function name for parseProtocol");
  else {
    func = lexer.getId();
    Token nexttok = lexer.getNextToken();
    if (nexttok != tok_parenthese_open)
      this->errorAt(loc, "expect '(' for parseProtocol");
    else {
      nexttok = lexer.getNextToken();
      while (nexttok != tok_parenthese_close) {
        if (nexttok == tok_comma) {
          nexttok = lexer.getNextToken();
          continue;
        }
        /// TODO: Should process struct type here
        for (const Struct &stru : structs)
          if (stru.getName() == lexer.getId())
            this->errorAt(loc, "Not support struct type for now");
        if (nexttok != tok_identifier)
          this->errorAt(loc, "expect variable name for parseProtocol");
        else
          args.push_back(
              std::make_unique<VariableExpr>(lexer.getLoc(), lexer.getId()));
        if ((nexttok = lexer.getNextToken()) != tok_comma &&
            nexttok != tok_parenthese_close)
          this->errorAt(loc, "expect ',' or ')' for parseProtocol");
      }
      return std::make_unique<Protocol>(std::move(args), func);
    }
  }
}

std::unique_ptr<VarDefExpr> Parser::parseVarDef() {
  Location loc = lexer.getLoc();
  if (lexer.getTok() != tok_var)
    this->errorAt(loc, "expect 'var' for parseVarDef");
  else {
    Token tok = lexer.getNextToken();
    std::string name = lexer.getId();
    tok = lexer.getNextToken();
    if (tok == tok_eq) {
      lexer.getNextToken();
      std::unique_ptr<Expr> expr = parseExpr();
      std::vector<int64_t> shape;
      shape = TensorExpr::classof(expr.get())
                  ? static_cast<TensorExpr *>(expr.get())->getShape()
                  : std::vector<int64_t>();
      return std::make_unique<VarDefExpr>(loc, name, std::move(expr),
                                          std::move(shape));
    } else if (tok == tok_less) {
      Token nexttok = lexer.getNextToken();
      VarType vartype;
      while (nexttok != tok_greater) {
        if (nexttok == tok_comma) {
          nexttok = lexer.getNextToken();
          continue;
        }
        if (nexttok != tok_number)
          this->errorAt(loc, "expect number for parseVarDef");
        else
          vartype.shape.push_back(lexer.getNum());
        if ((nexttok = lexer.getNextToken()) != tok_comma &&
            nexttok != tok_greater)
          this->errorAt(loc, "expect ',' or '>' for parseVarDef");
      }
      nexttok = lexer.getNextToken();
      if (nexttok != tok_eq)
        this->errorAt(loc, "expect '=' for parseVarDef");
      else {
        lexer.getNextToken();
        std::unique_ptr<Expr> expr = parseExpr();
        return std::make_unique<VarDefExpr>(loc, name, std::move(expr),
                                            vartype.shape);
      }
    }
  }
}

std::unique_ptr<ReturnExpr> Parser::parseReturn() {
  Location loc = lexer.getLoc();
  if (lexer.getTok() != tok_return)
    this->errorAt(loc, "expect 'return' for parseReturn");
  else {
    lexer.getNextToken();
    std::unique_ptr<Expr> expr = parseExpr();
    return std::make_unique<ReturnExpr>(loc, std::move(expr));
  }
}

std::unique_ptr<CallExpr> Parser::parseCall(bool hasConsumedIdentifier,
                                            std::string &&id) {
  Location loc = lexer.getLoc();
  if (!hasConsumedIdentifier && lexer.getTok() != tok_identifier)
    this->errorAt(loc, "expect identifier for parseCall");
  else {
    std::string func = hasConsumedIdentifier ? id : lexer.getId();
    std::vector<std::unique_ptr<Expr>> args;
    Token nexttok =
        hasConsumedIdentifier ? lexer.getTok() : lexer.getNextToken();
    if (nexttok != tok_parenthese_open)
      this->errorAt(loc, "expect '(' for parseCall");
    else {
      nexttok = lexer.getNextToken();
      while (nexttok != tok_parenthese_close) {
        if (nexttok == tok_comma) {
          nexttok = lexer.getNextToken();
          continue;
        }
        if (nexttok != tok_identifier) {
          if (nexttok == tok_sbracket_open) {
            args.push_back(parseTensor());
            return std::make_unique<CallExpr>(loc, func, std::move(args));
          } else
            this->errorAt(
                loc,
                "expect variable name, function name or tensor for parseCall");
        } else {
          Location loc = lexer.getLoc();
          std::string id = lexer.getId();
          if ((nexttok = lexer.getNextToken()) == tok_parenthese_open) {
            args.push_back(parseCall(true, std::string(id)));
            nexttok = lexer.getNextToken();
          } else {
            args.push_back(std::make_unique<VariableExpr>(loc, id));
          }
        }
        if ((nexttok = lexer.getTok()) != tok_comma &&
            nexttok != tok_parenthese_close)
          this->errorAt(loc, "expect ',' or ')' for parseCall");
      }
      return std::make_unique<CallExpr>(loc, func, std::move(args));
    }
  }
}

std::unique_ptr<Expr> Parser::parseLhs() {
  Location loc = lexer.getLoc();
  switch (lexer.getTok()) {
  case tok_identifier: {
    Location loc = lexer.getLoc();
    std::string id = lexer.getId();
    std::unique_ptr<Expr> result;
    if (lexer.getNextToken() == tok_parenthese_open) {
      result = parseCall(true, std::string(id));
      lexer.getNextToken();
    } else
      result = std::make_unique<VariableExpr>(loc, id);
    return result;
  }
  default:
    this->errorAt(loc, "expect identifier for parseLhs");
  }
}

std::unique_ptr<Expr> Parser::parseRhs() {
  Location loc = lexer.getLoc();
  Token nexttok = lexer.getNextToken();
  switch (nexttok) {
  case tok_identifier:
    return parseCall();
  default:
    this->errorAt(loc, "expect identifier for parseRhs");
  }
}

std::vector<double> Parser::parseSbracket() {
  std::vector<double> values;
  while (lexer.getTok() == tok_number) {
    values.push_back(lexer.getNum());
    if (lexer.getNextToken() != tok_comma &&
        lexer.getTok() != tok_sbracket_close)
      this->errorAt(lexer.getLoc(), "expect ',' or ']' for parseSbracket");
    if (lexer.getTok() == tok_sbracket_close)
      break;
    lexer.getNextToken();
  }
  return values;
}

std::unique_ptr<Expr> Parser::parseTensor() {
  Location loc = lexer.getLoc();
  if (lexer.getTok() != tok_sbracket_open)
    this->errorAt(loc, "expect '[' for parseTensor");
  else {
    // If the next token is also tok_sbracket_open, this is a 2D tensor.
    if (lexer.getNextToken() == tok_sbracket_open) {
      std::vector<std::vector<double>> allvalues;
      while (lexer.getTok() == tok_sbracket_open) {
        if (lexer.getNextToken() != tok_number)
          this->errorAt(lexer.getLoc(), "expect number for parseTensor");
        std::vector<double> values = parseSbracket();
        allvalues.push_back(std::move(values));
        if (lexer.getNextToken() != tok_comma &&
            lexer.getTok() != tok_sbracket_close)
          this->errorAt(lexer.getLoc(), "expect ',' or ']' for parseTensor");
        if (lexer.getTok() == tok_sbracket_close)
          break;
        lexer.getNextToken();
      }
      return std::make_unique<TensorExpr>(
          loc, std::move(allvalues),
          std::vector<int64_t>{static_cast<int64_t>(allvalues.size()),
                               static_cast<int64_t>(allvalues[0].size())});
    }
    // If the next token is a number, this is a 1D tensor.
    else if (lexer.getTok() == tok_number) {
      std::vector<std::vector<double>> allvalues;
      std::vector<double> values = parseSbracket();
      std::vector<int64_t> shape = {1, static_cast<int64_t>(values.size())};
      allvalues.push_back(std::move(values));
      return std::make_unique<TensorExpr>(loc, std::move(allvalues),
                                          std::move(shape));
    }
  }
}

std::unique_ptr<Expr> Parser::parseExpr() {
  Location loc = lexer.getLoc();
  Token nexttok = lexer.getTok();
  switch (nexttok) {
  case tok_return:
    return parseReturn();
  case tok_var:
    return parseVarDef();
  case tok_identifier: {
    std::unique_ptr<Expr> lhs = parseLhs();
    if (lexer.getTok() != tok_semicolon) {
      switch (lexer.getTok()) {
      case tok_mul: {
        std::unique_ptr<Expr> rhs = parseRhs();
        return std::make_unique<MulExpr>(loc, std::move(lhs), std::move(rhs));
      }
      }
    }
    return lhs;
  }
  case tok_sbracket_open:
    return parseTensor();
  case tok_number:
    return std::make_unique<TensorExpr>(
        loc, std::vector<std::vector<double>>{{lexer.getNum()}},
        std::vector<int64_t>{1, 1});
  default:
    this->errorAt(loc, "unexpected token for parseExpr");
  }
}

std::unique_ptr<ExprList> Parser::parseFuncBody() {
  Location loc = lexer.getLoc();
  std::vector<std::unique_ptr<Expr>> exprs;
  Token nexttok = lexer.getNextToken();
  if (nexttok != tok_bracket_open)
    this->errorAt(loc, "expect '{' for parseFuncBody");
  else {
    nexttok = lexer.getNextToken();
    while (nexttok != tok_bracket_close) {
      std::unique_ptr<Expr> expr = parseExpr();
      if (lexer.getTok() != tok_semicolon)
        if ((nexttok = lexer.getNextToken()) != tok_semicolon)
          this->errorAt(loc, "expect ';' for parseFuncBody");
      exprs.push_back(std::move(expr));
      nexttok = lexer.getNextToken();
    }
    return std::make_unique<ExprList>(loc, std::move(exprs));
  }
}

Struct Parser::parseStruct() {
  Location loc = lexer.getLoc();
  Token nexttok = lexer.getNextToken();
  if (nexttok != tok_identifier)
    this->errorAt(loc, "expect identifier for parseStruct");
  else {
    std::string name = lexer.getId();
    std::vector<std::unique_ptr<VariableExpr>> vars;
    nexttok = lexer.getNextToken();
    if (nexttok != tok_bracket_open)
      this->errorAt(loc, "expect '{' for parseStruct");
    else {
      nexttok = lexer.getNextToken();
      while (nexttok != tok_bracket_close) {
        if (nexttok != tok_var)
          this->errorAt(loc, "expect 'var' for parseStruct");
        else {
          if ((nexttok = lexer.getNextToken()) != tok_identifier)
            this->errorAt(loc, "expect identifier for parseStruct");
          else {
            std::unique_ptr<VariableExpr> var =
                std::make_unique<VariableExpr>(lexer.getLoc(), lexer.getId());
            vars.push_back(std::move(var));
          }
          if ((nexttok = lexer.getNextToken()) != tok_semicolon)
            this->errorAt(loc, "expect ';' for parseStruct");
          else
            nexttok = lexer.getNextToken();
        }
      }
      structs.emplace_back(name, std::move(vars));
      return Struct(name, std::move(vars));
    }
  }
}

Function Parser::parseFunction() {
  Location loc = lexer.getLoc();
  std::unique_ptr<Protocol> proto = parseProtocol();
  std::unique_ptr<ExprList> body = parseFuncBody();
  return Function(std::move(proto), std::move(body));
}

void Parser::errorAt(const Location &loc, const std::string &msg) {
  std::strstream errmsg;
  errmsg << "Error: " << msg << " (Location: " << loc.getRow() << ":"
         << loc.getCol() << ")\n";
  throw UnexceptedTokenError(loc, errmsg.str());
}

} // namespace mytoy
