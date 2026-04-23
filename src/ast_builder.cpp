#include "ast_builder.h"

#include <iostream>

using namespace std;
// recursive descent parser and syntax tree.
ASTBuilder::ASTBuilder(vector<Token> tokenList) {
  tokens = tokenList;
  currentIndex = 0;
  root = nullptr;
}

ASTBuilder::~ASTBuilder() { delete root; }

ASTNode* ASTBuilder::buildAST() {
  delete root;
  root = nullptr;
  currentIndex = 0;
  root = parseProgram();
  return root;
}

void ASTBuilder::printAST(ASTNode* node, int indent) {
  if (!node) {
    return;
  }

  for (int i = 0; i < indent; i++) {
    cout << "  ";
  }

  cout << nodeTypeToString(node->type);
  if (!node->value.empty()) {
    cout << ": " << node->value;
  }
  if (node->line > 0) {
    cout << " (line " << node->line << ")";
  }
  cout << "\n";

  for (ASTNode* child : node->children) {
    printAST(child, indent + 1);
  }
}

Token ASTBuilder::currentToken() {
  if (currentIndex < (int)(tokens.size())) {
    return tokens[currentIndex];
  }

  Token eofToken;
  eofToken.type = TOKEN_EOF;
  eofToken.line = 0;
  eofToken.column = 0;
  return eofToken;
}

Token ASTBuilder::peekToken(int offset) {
  int index = currentIndex + offset;
  if (index >= 0 && index < (int)(tokens.size())) {
    return tokens[index];
  }

  Token eofToken;
  eofToken.type = TOKEN_EOF;
  eofToken.line = 0;
  eofToken.column = 0;
  return eofToken;
}

bool ASTBuilder::isAtEnd() { return currentToken().type == TOKEN_EOF; }

bool ASTBuilder::check(int type) { return currentToken().type == type; }

bool ASTBuilder::match(int type) {
  if (!check(type)) {
    return false;
  }

  advance();
  return true;
}

bool ASTBuilder::isTypeToken(const Token& token) {
  return token.type == TOKEN_INT || token.type == TOKEN_VOID ||
         token.type == TOKEN_IDENTIFIER;
}

bool ASTBuilder::isFunctionSignature() {
  if (!isTypeToken(currentToken())) {
    return false;
  }

  if (peekToken().type != TOKEN_IDENTIFIER) {
    return false;
  }

  return peekToken(2).type == TOKEN_LPAREN;
}

void ASTBuilder::advance() {
  if (currentIndex < (int)(tokens.size())) {
    currentIndex++;
  }
}

string ASTBuilder::nodeTypeToString(int type) {
  switch (type) {
    case NODE_PROGRAM:
      return "Program";
    case NODE_FUNCTION:
      return "Function";
    case NODE_VARIABLE:
      return "Variable";
    case NODE_IF:
      return "If";
    case NODE_WHILE:
      return "While";
    case NODE_FOR:
      return "For";
    case NODE_RETURN:
      return "Return";
    case NODE_EXPRESSION:
      return "Expression";
    case NODE_IDENTIFIER:
      return "Identifier";
    case NODE_NUMBER:
      return "Number";
    case NODE_BLOCK:
      return "Block";
    case NODE_STRING:
      return "String";
    case NODE_CALL:
      return "Call";
    case NODE_PARAMETER:
      return "Parameter";
    case NODE_UNARY:
      return "Unary";
    default:
      return "Unknown";
  }
}

ASTNode* ASTBuilder::parseBlock() {
  if (!match(TOKEN_LBRACE)) {
    return nullptr;
  }

  ASTNode* block = new ASTNode(NODE_BLOCK, "block", currentToken().line);
  while (!isAtEnd() && !check(TOKEN_RBRACE)) {
    int startIndex = currentIndex;
    ASTNode* statement = parseStatement();
    if (statement) {
      block->addChild(statement);
    } else if (currentIndex == startIndex) {
      advance();
    }
  }

  match(TOKEN_RBRACE);
  return block;
}

ASTNode* ASTBuilder::parseProgram() {
  ASTNode* program = new ASTNode(NODE_PROGRAM, "root", currentToken().line);

  while (!isAtEnd()) {
    ASTNode* node = nullptr;
    int startIndex = currentIndex;

    if (isFunctionSignature()) {
      node = parseFunction();
    } else {
      node = parseStatement();
    }

    if (node) {
      program->addChild(node);
    } else if (currentIndex == startIndex) {
      advance();
    }
  }

  return program;
}

ASTNode* ASTBuilder::parseFunction() {
  Token returnType = currentToken();
  advance();

  Token functionName = currentToken();
  if (functionName.type != TOKEN_IDENTIFIER) {
    return nullptr;
  }
  advance();

  if (!match(TOKEN_LPAREN)) {
    return nullptr;
  }

  ASTNode* functionNode =
      new ASTNode(NODE_FUNCTION, returnType.value + " " + functionName.value,
                  functionName.line);

  while (!isAtEnd() && !check(TOKEN_RPAREN)) {
    Token parameterType = currentToken();
    if (!isTypeToken(parameterType)) {
      advance();
      continue;
    }
    advance();

    string parameterValue = parameterType.value;
    if (check(TOKEN_IDENTIFIER)) {
      parameterValue += " " + currentToken().value;
      advance();
    }

    functionNode->addChild(
        new ASTNode(NODE_PARAMETER, parameterValue, parameterType.line));

    if (!match(TOKEN_COMMA)) {
      break;
    }
  }

  match(TOKEN_RPAREN);

  if (check(TOKEN_LBRACE)) {
    functionNode->addChild(parseBlock());
  } else {
    match(TOKEN_SEMICOLON);
  }

  return functionNode;
}

ASTNode* ASTBuilder::parseStatement() {
  if (check(TOKEN_LBRACE)) {
    return parseBlock();
  }

  if (currentToken().type == TOKEN_IDENTIFIER &&
      currentToken().value == "using") {
    while (!isAtEnd() && !check(TOKEN_SEMICOLON)) {
      advance();
    }
    match(TOKEN_SEMICOLON);
    return nullptr;
  }

  if (check(TOKEN_IF)) {
    return parseIfStatement();
  }

  if (check(TOKEN_WHILE)) {
    return parseWhileStatement();
  }

  if (check(TOKEN_FOR)) {
    return parseForStatement();
  }

  if (check(TOKEN_RETURN)) {
    return parseReturnStatement();
  }

  if (isTypeToken(currentToken()) && peekToken().type == TOKEN_IDENTIFIER &&
      peekToken(2).type != TOKEN_LPAREN) {
    return parseVariableDeclaration();
  }

  return parseExpressionStatement();
}

ASTNode* ASTBuilder::parseIfStatement() {
  Token ifToken = currentToken();
  advance();
  match(TOKEN_LPAREN);

  ASTNode* node = new ASTNode(NODE_IF, "if", ifToken.line);
  ASTNode* condition = parseExpression();
  if (condition) {
    node->addChild(condition);
  }

  match(TOKEN_RPAREN);

  ASTNode* thenBranch = parseStatement();
  if (thenBranch) {
    node->addChild(thenBranch);
  }

  if (match(TOKEN_ELSE)) {
    ASTNode* elseBranch = parseStatement();
    if (elseBranch) {
      node->addChild(elseBranch);
    }
  }

  return node;
}

ASTNode* ASTBuilder::parseWhileStatement() {
  Token whileToken = currentToken();
  advance();
  match(TOKEN_LPAREN);

  ASTNode* node = new ASTNode(NODE_WHILE, "while", whileToken.line);
  ASTNode* condition = parseExpression();
  if (condition) {
    node->addChild(condition);
  }

  match(TOKEN_RPAREN);

  ASTNode* body = parseStatement();
  if (body) {
    node->addChild(body);
  }

  return node;
}

ASTNode* ASTBuilder::parseForStatement() {
  Token forToken = currentToken();
  advance();
  match(TOKEN_LPAREN);

  ASTNode* node = new ASTNode(NODE_FOR, "for", forToken.line);

  if (!check(TOKEN_SEMICOLON)) {
    ASTNode* initializer = nullptr;
    if (isTypeToken(currentToken()) && peekToken().type == TOKEN_IDENTIFIER &&
        peekToken(2).type != TOKEN_LPAREN) {
      initializer = parseVariableDeclaration();
    } else {
      initializer = parseExpression();
      match(TOKEN_SEMICOLON);
    }

    if (initializer) {
      node->addChild(initializer);
    }
  } else {
    match(TOKEN_SEMICOLON);
  }

  if (!check(TOKEN_SEMICOLON)) {
    ASTNode* condition = parseExpression();
    if (condition) {
      node->addChild(condition);
    }
  }
  match(TOKEN_SEMICOLON);

  if (!check(TOKEN_RPAREN)) {
    ASTNode* update = parseExpression();
    if (update) {
      node->addChild(update);
    }
  }
  match(TOKEN_RPAREN);

  ASTNode* body = parseStatement();
  if (body) {
    node->addChild(body);
  }

  return node;
}

ASTNode* ASTBuilder::parseReturnStatement() {
  Token returnToken = currentToken();
  advance();

  ASTNode* node = new ASTNode(NODE_RETURN, "return", returnToken.line);
  if (!check(TOKEN_SEMICOLON)) {
    ASTNode* expression = parseExpression();
    if (expression) {
      node->addChild(expression);
    }
  }

  match(TOKEN_SEMICOLON);
  return node;
}

ASTNode* ASTBuilder::parseVariableDeclaration() {
  Token typeToken = currentToken();
  advance();

  Token nameToken = currentToken();
  if (nameToken.type != TOKEN_IDENTIFIER) {
    return nullptr;
  }
  advance();

  ASTNode* node = new ASTNode(
      NODE_VARIABLE, typeToken.value + " " + nameToken.value, nameToken.line);

  if (match(TOKEN_ASSIGN)) {
    ASTNode* assignmentNode = new ASTNode(NODE_EXPRESSION, "=", nameToken.line);
    assignmentNode->addChild(
        new ASTNode(NODE_IDENTIFIER, nameToken.value, nameToken.line));

    ASTNode* value = parseExpression();
    if (value) {
      assignmentNode->addChild(value);
    }
    node->addChild(assignmentNode);
  }

  match(TOKEN_SEMICOLON);
  return node;
}

ASTNode* ASTBuilder::parseExpressionStatement() {
  if (match(TOKEN_SEMICOLON)) {
    return new ASTNode(NODE_EXPRESSION, "empty", currentToken().line);
  }

  ASTNode* expression = parseExpression();
  match(TOKEN_SEMICOLON);
  return expression;
}

ASTNode* ASTBuilder::parseExpression() { return parseAssignment(); }

ASTNode* ASTBuilder::parseAssignment() {
  ASTNode* left = parseLogicalOr();
  if (!left) {
    return nullptr;
  }

  if (match(TOKEN_ASSIGN)) {
    ASTNode* node = new ASTNode(NODE_EXPRESSION, "=", currentToken().line);
    node->addChild(left);

    ASTNode* right = parseAssignment();
    if (right) {
      node->addChild(right);
    }
    return node;
  }

  return left;
}

ASTNode* ASTBuilder::parseLogicalOr() {
  ASTNode* left = parseLogicalAnd();

  while (match(TOKEN_OR)) {
    ASTNode* node = new ASTNode(NODE_EXPRESSION, "||", currentToken().line);
    node->addChild(left);
    node->addChild(parseLogicalAnd());
    left = node;
  }

  return left;
}

ASTNode* ASTBuilder::parseLogicalAnd() {
  ASTNode* left = parseEquality();

  while (match(TOKEN_AND)) {
    ASTNode* node = new ASTNode(NODE_EXPRESSION, "&&", currentToken().line);
    node->addChild(left);
    node->addChild(parseEquality());
    left = node;
  }

  return left;
}

ASTNode* ASTBuilder::parseEquality() {
  ASTNode* left = parseComparison();

  while (check(TOKEN_EQUAL) || check(TOKEN_NOT_EQUAL)) {
    Token op = currentToken();
    advance();

    ASTNode* node = new ASTNode(NODE_EXPRESSION, op.value, op.line);
    node->addChild(left);
    node->addChild(parseComparison());
    left = node;
  }

  return left;
}

ASTNode* ASTBuilder::parseComparison() {
  ASTNode* left = parseAdditive();

  while (check(TOKEN_LESS) || check(TOKEN_LESS_EQUAL) || check(TOKEN_GREATER) ||
         check(TOKEN_GREATER_EQUAL)) {
    Token op = currentToken();
    advance();

    ASTNode* node = new ASTNode(NODE_EXPRESSION, op.value, op.line);
    node->addChild(left);
    node->addChild(parseAdditive());
    left = node;
  }

  return left;
}

ASTNode* ASTBuilder::parseAdditive() {
  ASTNode* left = parseMultiplicative();

  while (check(TOKEN_PLUS) || check(TOKEN_MINUS)) {
    Token op = currentToken();
    advance();

    ASTNode* node = new ASTNode(NODE_EXPRESSION, op.value, op.line);
    node->addChild(left);
    node->addChild(parseMultiplicative());
    left = node;
  }

  return left;
}

ASTNode* ASTBuilder::parseMultiplicative() {
  ASTNode* left = parseUnary();

  while (check(TOKEN_MULTIPLY) || check(TOKEN_DIVIDE) || check(TOKEN_MODULO)) {
    Token op = currentToken();
    advance();

    ASTNode* node = new ASTNode(NODE_EXPRESSION, op.value, op.line);
    node->addChild(left);
    node->addChild(parseUnary());
    left = node;
  }

  return left;
}

ASTNode* ASTBuilder::parseUnary() {
  if (check(TOKEN_NOT) || check(TOKEN_MINUS) || check(TOKEN_PLUS) ||
      check(TOKEN_INCREMENT) || check(TOKEN_DECREMENT)) {
    Token op = currentToken();
    advance();

    ASTNode* node = new ASTNode(NODE_UNARY, op.value, op.line);
    node->addChild(parseUnary());
    return node;
  }

  return parsePostfix();
}

ASTNode* ASTBuilder::parsePostfix() {
  ASTNode* node = parsePrimary();
  if (!node) {
    return nullptr;
  }

  while (check(TOKEN_INCREMENT) || check(TOKEN_DECREMENT)) {
    Token op = currentToken();
    advance();

    ASTNode* postfixNode = new ASTNode(NODE_UNARY, "post" + op.value, op.line);
    postfixNode->addChild(node);
    node = postfixNode;
  }

  return node;
}

ASTNode* ASTBuilder::parsePrimary() {
  Token token = currentToken();

  if (match(TOKEN_NUMBER)) {
    return new ASTNode(NODE_NUMBER, token.value, token.line);
  }

  if (match(TOKEN_STRING)) {
    return new ASTNode(NODE_STRING, token.value, token.line);
  }

  if (match(TOKEN_IDENTIFIER)) {
    if (match(TOKEN_LPAREN)) {
      ASTNode* callNode = new ASTNode(NODE_CALL, token.value, token.line);

      while (!isAtEnd() && !check(TOKEN_RPAREN)) {
        ASTNode* argument = parseExpression();
        if (argument) {
          callNode->addChild(argument);
        }

        if (!match(TOKEN_COMMA)) {
          break;
        }
      }

      match(TOKEN_RPAREN);
      return callNode;
    }

    return new ASTNode(NODE_IDENTIFIER, token.value, token.line);
  }

  if (match(TOKEN_LPAREN)) {
    ASTNode* expression = parseExpression();
    match(TOKEN_RPAREN);
    return expression;
  }

  if (token.type != TOKEN_EOF) {
    advance();
    return new ASTNode(NODE_EXPRESSION, token.value, token.line);
  }

  return nullptr;
}
