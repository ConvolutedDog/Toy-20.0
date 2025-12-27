#pragma once

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"

namespace mytoy {

class Location {
public:
  Location(llvm::StringRef path, int row, int col)
      : path(path.str()), row(row), col(col) {}

  llvm::StringRef getPath() const { return path; }
  int getRow() const { return row; }
  int getCol() const { return col; }

public:
  std::string path;
  int row;
  int col;
};

enum Token {
  tok_eof = -1,
  tok_def = -2,
  tok_var = -3,
  tok_return = -4,
  tok_identifier = -5,
  tok_number = -6,
  tok_struct = -7,

  tok_comma = ',',
  tok_semicolon = ';',
  tok_parenthese_open = '(',
  tok_parenthese_close = ')',
  tok_bracket_open = '{',
  tok_bracket_close = '}',
  tok_sbracket_open = '[',
  tok_sbracket_close = ']',
  tok_less = '<',
  tok_greater = '>',

  tok_mul = '*',
  tok_add = '+',
  tok_sub = '-',
  tok_eq = '=',
};

class TokenConverter {
public:
  /// Get the string representation of a token.
  static llvm::StringRef toString(Token tok);

  /// Get the token of a string.
  static Token fromString(const llvm::StringRef &str);

  /// Get the token of a character.
  static Token fromChar(const char &c);

private:
  /// The character to token map.
  static const llvm::SmallDenseMap<char, Token> &getCharToTokenMap();
  /// The string to token map.
  static const llvm::StringMap<Token> &getStringToTokenMap();
  /// The token to string map.
  static const llvm::SmallDenseMap<Token, llvm::StringRef> &
  getTokenToStringMap();
};

class Lexer {
public:
  Lexer(const llvm::StringRef &buffer, const llvm::StringRef &filename)
      : bufferImpl(buffer), buffer(bufferImpl), currLoc{filename, 1, 1} {}

  /// Get the next token.
  Token getNextToken() { return currTok = getNextTokenImpl(); }

  /// Get the location of current token.
  const Location &getLoc() const { return currLoc; }

  /// Get the current token/identifier/number.
  const Token &getTok() const { return currTok; }
  const std::string &getId() const { return currId; }
  double getNum() const { return currNum; }

  /// Dump the current state of lexer.
  void dumpCurrStat(std::ostream &os = std::cout);

private:
  /// The implementation of getNextToken().
  Token getNextTokenImpl();

  /// Advance one character.
  Token advanceOneChar();
  /// Return character.
  void returnChar();

  /// The current location.
  Location currLoc;

  /// The current token/identifier/number.
  Token currTok = tok_eof;
  std::string currId = {};
  double currNum = 0;

  /// The buffer to lex.
  const std::string bufferImpl;
  llvm::StringRef buffer;
  /// The current position in the buffer.
  int ichar = 0;
};

} // namespace mytoy
