#ifndef AST_BUILDER_H
#define AST_BUILDER_H

#include <string>
#include <vector>

#include "lexer.h"

using namespace std;

#define NODE_PROGRAM 1
#define NODE_FUNCTION 2
#define NODE_VARIABLE 3
#define NODE_IF 4
#define NODE_WHILE 5
#define NODE_FOR 6
#define NODE_RETURN 7
#define NODE_EXPRESSION 8
#define NODE_IDENTIFIER 9
#define NODE_NUMBER 10

struct ASTNode {
  int type;
  string value;
  vector<ASTNode*> children;
  int line;

  ASTNode(int t, string v = "", int l = 0) {
    type = t;
    value = v;
    line = l;
  }

  void addChild(ASTNode* child) { children.push_back(child); }
};

class ASTBuilder {
 public:
  ASTBuilder(vector<Token> tokens);
  ~ASTBuilder();

  ASTNode* buildAST();
  void printAST(ASTNode* node, int indent = 0);

 private:
  vector<Token> tokens;
  int currentIndex;

  Token currentToken();
  void advance();
  ASTNode* parseProgram();
  ASTNode* parseFunction();
  ASTNode* parseStatement();
  ASTNode* parseExpression();
};

#endif