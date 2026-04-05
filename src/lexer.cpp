#include "lexer.h"

#include <cctype>

using namespace std;

Lexer::Lexer(string sourceCode)
    : source(sourceCode), position(0), line(1), column(1) {}

Lexer::~Lexer() {}

vector<Token> Lexer::tokenize() {
  vector<Token> tokens;
  while (true) {
    Token token = nextToken();
    tokens.push_back(token);
    if (token.type == TOKEN_EOF) {
      break;
    }
  }
  return tokens;
}

Token Lexer::nextToken() {
  skipWhitespace();

  Token token;
  token.line = line;
  token.column = column;
  token.value = "";

  if (!hasMoreTokens()) {
    token.type = TOKEN_EOF;
    return token;
  }

  char ch = currentChar();

  if (isalpha(static_cast<unsigned char>(ch)) || ch == '_') {
    return readIdentifierOrKeyword();
  }

  if (isdigit(static_cast<unsigned char>(ch))) {
    return readNumber();
  }

  if (ch == '/' && position + 1 < static_cast<int>(source.size())) {
    char next = source[position + 1];
    if (next == '/') {
      while (hasMoreTokens() && currentChar() != '\n') {
        advance();
      }
      return nextToken();
    }
    if (next == '*') {
      advance();
      advance();
      while (hasMoreTokens()) {
        if (currentChar() == '*' &&
            position + 1 < static_cast<int>(source.size()) &&
            source[position + 1] == '/') {
          advance();
          advance();
          break;
        }
        advance();
      }
      return nextToken();
    }
  }

  if (ch == '"') {
    token.type = TOKEN_STRING;
    advance();
    while (hasMoreTokens() && currentChar() != '"') {
      token.value += currentChar();
      advance();
    }
    if (hasMoreTokens() && currentChar() == '"') {
      advance();
    }
    return token;
  }

  switch (ch) {
    case '(':
      token.type = TOKEN_LPAREN;
      token.value = "(";
      advance();
      return token;
    case ')':
      token.type = TOKEN_RPAREN;
      token.value = ")";
      advance();
      return token;
    case '{':
      token.type = TOKEN_LBRACE;
      token.value = "{";
      advance();
      return token;
    case '}':
      token.type = TOKEN_RBRACE;
      token.value = "}";
      advance();
      return token;
    case ';':
      token.type = TOKEN_SEMICOLON;
      token.value = ";";
      advance();
      return token;
    case ',':
      token.type = TOKEN_COMMA;
      token.value = ",";
      advance();
      return token;
    case '=':
      token.type = TOKEN_ASSIGN;
      token.value = "=";
      advance();
      return token;
    case '+':
      token.type = TOKEN_PLUS;
      token.value = "+";
      advance();
      return token;
    case '-':
      token.type = TOKEN_MINUS;
      token.value = "-";
      advance();
      return token;
    case '*':
      token.type = TOKEN_MULTIPLY;
      token.value = "*";
      advance();
      return token;
    case '/':
      token.type = TOKEN_DIVIDE;
      token.value = "/";
      advance();
      return token;
    default:
      token.type = TOKEN_UNKNOWN;
      token.value = string(1, ch);
      advance();
      return token;
  }
}

bool Lexer::hasMoreTokens() { return position < static_cast<int>(source.size()); }

char Lexer::currentChar() {
  if (position >= source.size()) return '\0';
  return source[position];
}

void Lexer::advance() {
  if (position < source.length()) {
    if (source[position] == '\n') {
      line++;
      column = 1;
    } else {
      column++;
    }
    position++;
  }
}

void Lexer::skipWhitespace() {
  while (hasMoreTokens() &&
         isspace(static_cast<unsigned char>(currentChar()))) {
    advance();
  }
}

Token Lexer::readIdentifierOrKeyword() {
  Token token;
  token.line = line;
  token.column = column;

  while (hasMoreTokens()) {
    char ch = currentChar();
    if (isalnum(static_cast<unsigned char>(ch)) || ch == '_') {
      token.value += ch;
      advance();
    } else {
      break;
    }
  }

  if (token.value == "int")
    token.type = TOKEN_INT;
  else if (token.value == "void")
    token.type = TOKEN_VOID;
  else if (token.value == "return")
    token.type = TOKEN_RETURN;
  else if (token.value == "if")
    token.type = TOKEN_IF;
  else if (token.value == "else")
    token.type = TOKEN_ELSE;
  else if (token.value == "while")
    token.type = TOKEN_WHILE;
  else if (token.value == "for")
    token.type = TOKEN_FOR;
  else
    token.type = TOKEN_IDENTIFIER;

  return token;
}

Token Lexer::readNumber() {
  Token token;
  token.type = TOKEN_NUMBER;
  token.line = line;
  token.column = column;

  while (hasMoreTokens() && isdigit(static_cast<unsigned char>(currentChar()))) {
    token.value += currentChar();
    advance();
  }

  return token;
}
