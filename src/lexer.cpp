#include "lexer.h"

using namespace std;

Lexer::Lexer(string sourceCode)
    : source(sourceCode), position(0), line(1), column(1) {}

Lexer::~Lexer() {}

vector<Token> Lexer::tokenize() {
  vector<Token> tokens;
  //
  return tokens;
}

Token Lexer::nextToken() {
  Token token;
  token.type = TOKEN_EOF;
  return token;
}

bool Lexer::hasMoreTokens() { return position < source.size(); }

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

void Lexer::skipWhitespace() {}

Token Lexer::readIdentifierOrKeyword() {
  Token token;
  return token;
}

Token Lexer::readNumber() {
  Token token;
  return token;
}