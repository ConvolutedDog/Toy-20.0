#include "mytoy/Lexer.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include <string>
#include <unordered_map>

namespace mytoy {

static std::ostream &operator<<(std::ostream &os, const Location &loc) {
  std::string str = loc.getPath().str() + ":" + std::to_string(loc.getRow()) +
                    ":" + std::to_string(loc.getCol());
  return os << str;
}

static const llvm::StringMap<Token> string2Token = {
    {"eof", tok_eof},
    {"def", tok_def},
    {"var", tok_var},
    {"return", tok_return},
    {"identifier", tok_identifier},
    {"number", tok_number},
    {"struct", tok_struct},

    {",", tok_comma},
    {";", tok_semicolon},
    {"(", tok_parenthese_open},
    {")", tok_parenthese_close},
    {"{", tok_bracket_open},
    {"}", tok_bracket_close},
    {"[", tok_sbracket_open},
    {"]", tok_sbracket_close},
    {"<", tok_less},
    {">", tok_greater},

    {"*", tok_mul},
    {"+", tok_add},
    {"-", tok_sub},
    {"=", tok_eq},
};

static const llvm::SmallDenseMap<Token, llvm::StringRef> token2String = []() {
  llvm::SmallDenseMap<Token, llvm::StringRef> result;
  for (const auto &[str, tok] : string2Token) {
    result[tok] = str;
  }
  return result;
}();

static const llvm::SmallDenseMap<char, Token> char2Token = []() {
  llvm::SmallDenseMap<char, Token> result;
  for (const auto &[str, tok] : string2Token) {
    if (str.size() == 1) {
      result[str[0]] = tok;
    }
  }
  return result;
}();

llvm::StringRef TokenConverter::toString(Token tok) {
  auto it = getTokenToStringMap().find(tok);
  return it != getTokenToStringMap().end() ? it->second : "unknown token";
}

Token TokenConverter::fromString(const llvm::StringRef &str) {
  auto it = getStringToTokenMap().find(str);
  return it != getStringToTokenMap().end() ? it->second : tok_eof;
}

Token TokenConverter::fromChar(const char &c) {
  auto it = getCharToTokenMap().find(c);
  return it != getCharToTokenMap().end() ? it->second : tok_eof;
}

const llvm::SmallDenseMap<char, Token> &TokenConverter::getCharToTokenMap() {
  return char2Token;
}

const llvm::StringMap<Token> &TokenConverter::getStringToTokenMap() {
  return string2Token;
}

const llvm::SmallDenseMap<Token, llvm::StringRef> &
TokenConverter::getTokenToStringMap() {
  return token2String;
}

static std::ostream &operator<<(std::ostream &os, const Token &tok) {
  return os << TokenConverter::toString(tok).str();
}

static bool isNumber(const std::string &str) {
  bool decimalPointFound = false;
  for (char ch : str) {
    if (ch == '.') {
      if (decimalPointFound)
        return false;
      decimalPointFound = true;
    } else if (!std::isdigit(ch)) {
      return false;
    }
  }
  return true;
}

void Lexer::dumpCurrStat(std::ostream &os) {
  os << "Token: " << std::left << std::setw(10) << currTok
     << " | currId: " << std::setw(18) << currId
     << " | currNum: " << std::setw(3) << currNum
     << " | currLoc: " << std::setw(30) << getLoc() << "\n";
}

Token Lexer::getNextTokenImpl() {
  currId.clear();
  currNum = 0;

  while (true) {
    if (ichar >= buffer.size()) {
      currTok = tok_eof;
      break;
    }

    switch (buffer[ichar]) {
    case ',':
    case ';':
    case '(':
    case ')':
    case '{':
    case '}':
    case '[':
    case ']':
    case '<':
    case '>':
    case '*':
    case '+':
    case '-':
    case '=':
      return advanceOneChar();
    case '#':
      while (buffer[ichar] != '\n')
        ichar++;
      break;
    case '\n':
      returnChar();
      break;
    case ' ':
      currLoc.col++;
      ichar++;
      break;
    default: {
      while (buffer[ichar] != '(' && buffer[ichar] != ')' &&
             buffer[ichar] != '[' && buffer[ichar] != ']' &&
             buffer[ichar] != '<' && buffer[ichar] != '>' &&
             buffer[ichar] != ',' && buffer[ichar] != ' ' &&
             buffer[ichar] != '\n' && buffer[ichar] != ';') {
        currId += buffer[ichar];
        currLoc.col++;
        ichar++;
      }

      Token tok;

      if (currId == "def") {
        tok = tok_def;
      } else if (currId == "var") {
        tok = tok_var;
      } else if (currId == "return") {
        tok = tok_return;
      } else if (currId == "struct") {
        tok = tok_struct;
      } else {
        // If currId a number, return number
        if (isNumber(currId)) {
          currNum = std::stod(currId);
          tok = tok_number;
        } else {
          tok = tok_identifier;
        }
      }

      return tok;
    }
    }
  }

  return currTok;
}

Token Lexer::advanceOneChar() {
  Token tok = TokenConverter::fromChar(buffer[ichar++]);
  currLoc.col++;
  return tok;
}

void Lexer::returnChar() {
  ichar++;
  currLoc.col = 0;
  currLoc.row++;
}

} // namespace mytoy
