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
#define NODE_BLOCK 11
#define NODE_STRING 12
#define NODE_CALL 13
#define NODE_PARAMETER 14
#define NODE_UNARY 15

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

  ~ASTNode() {
    for (ASTNode* child : children) {
      delete child;
    }
  }

  void addChild(ASTNode* child) {
    if (child) {
      children.push_back(child);
    }
  }
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
  ASTNode* root;

  Token currentToken();
  Token peekToken(int offset = 1);
  bool isAtEnd();
  bool check(int type);
  bool match(int type);
  bool isTypeToken(const Token& token);
  bool isFunctionSignature();
  void advance();
  string nodeTypeToString(int type);
  ASTNode* parseBlock();
  ASTNode* parseProgram();
  ASTNode* parseFunction();
  ASTNode* parseStatement();
  ASTNode* parseIfStatement();
  ASTNode* parseWhileStatement();
  ASTNode* parseForStatement();
  ASTNode* parseReturnStatement();
  ASTNode* parseVariableDeclaration();
  ASTNode* parseExpressionStatement();
  ASTNode* parseExpression();
  ASTNode* parseAssignment();
  ASTNode* parseLogicalOr();
  ASTNode* parseLogicalAnd();
  ASTNode* parseEquality();
  ASTNode* parseComparison();
  ASTNode* parseAdditive();
  ASTNode* parseMultiplicative();
  ASTNode* parseUnary();
  ASTNode* parsePostfix();
  ASTNode* parsePrimary();
};