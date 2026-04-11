#include <iostream>
#include <string>
#include <vector>

#include "ast_builder.h"
#include "lexer.h"

using namespace std;

int main() {
  const string sourceCode = R"(
int add(int a, int b) {
  int result = a + b;
  if (result > 10) {
    return result;
  }
  return b;
}

void tick() {
  for (int i = 0; i < 3; i++) {
    total = total + i;
  }
}
)";

  Lexer lexer(sourceCode);
  vector<Token> tokens = lexer.tokenize();

  ASTBuilder builder(tokens);
  ASTNode* root = builder.buildAST();

  if (!root) {
    cerr << "Failed to build AST\n";
    return 1;
  }

  if (root->children.size() != 2) {
    cerr << "Unexpected top-level node count: " << root->children.size()
         << "\n";
    return 1;
  }

  builder.printAST(root);
  cout << "\nAST builder smoke test passed.\n";
  return 0;
}
