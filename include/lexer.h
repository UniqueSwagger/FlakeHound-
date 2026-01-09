#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
using namespace std;
// Simple token types as integers
#define TOKEN_INT 1
#define TOKEN_VOID 2
#define TOKEN_RETURN 3
#define TOKEN_IF 4
#define TOKEN_ELSE 5
#define TOKEN_WHILE 6
#define TOKEN_FOR 7
#define TOKEN_IDENTIFIER 8
#define TOKEN_NUMBER 9
#define TOKEN_STRING 10
#define TOKEN_LPAREN 11
#define TOKEN_RPAREN 12
#define TOKEN_LBRACE 13
#define TOKEN_RBRACE 14
#define TOKEN_SEMICOLON 15
#define TOKEN_COMMA 16
#define TOKEN_ASSIGN 17
#define TOKEN_PLUS 18
#define TOKEN_MINUS 19
#define TOKEN_MULTIPLY 20
#define TOKEN_DIVIDE 21
#define TOKEN_EOF 22
#define TOKEN_UNKNOWN 23

struct Token {
  int type;
  string value;
  int line;
  int column;
};

class Lexer {
 public:
  Lexer(string sourceCode);
  ~Lexer();

  vector<Token> tokenize();
  Token nextToken();
  bool hasMoreTokens();

 private:
  string source;
  int position;
  int line;
  int column;

  char currentChar();
  void advance();
  void skipWhitespace();
  Token readIdentifierOrKeyword();
  Token readNumber();
};

#endif