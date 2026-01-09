#include "ast_builder.h"

using namespace std;

ASTBuilder::ASTBuilder(vector<Token> tokens)
    : tokens(tokens), currentIndex(0) {}

ASTBuilder::~ASTBuilder() {}

ASTNode* ASTBuilder::buildAST() { return nullptr; }

void ASTBuilder::printAST(ASTNode* node, int indent) {}

Token ASTBuilder::currentToken() {
  if (currentIndex < tokens.size()) {
    return tokens[currentIndex];
  }
  Token eofToken;
  eofToken.type = TOKEN_EOF;
  return eofToken;
}

void ASTBuilder::advance() {
  if (currentIndex < tokens.size()) {
    currentIndex++;
  }
}

ASTNode* ASTBuilder::parseProgram() { return nullptr; }
ASTNode* ASTBuilder::parseFunction() { return nullptr; }
ASTNode* ASTBuilder::parseStatement() { return nullptr; }
ASTNode* ASTBuilder::parseExpression() { return nullptr; }
