#pragma once
#include <string>

enum class TokenType {
  NUMBER,
  DECIMAL_LITERAL,
  IDENTIFIER,
  PLUS,
  MINUS,
  MULTIPLY,
  DIVIDE,
  POWER,
  EQUALS,
  LPAREN,
  RPAREN,
  LBRACKET,
  RBRACKET,
  COMMA,
  END
};

struct Token {
  TokenType type;
  std::string value;
};

enum class OutputMode { AUTO, DECIMAL, FRACTION };
