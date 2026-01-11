#include "cfg_generator.h"

#include <iostream>

#include "ast_builder.h"

using namespace std;

CFGGenerator::CFGGenerator(ASTNode* astRoot)
    : astRoot(astRoot), nodeIdCounter(0) {}

CFGGenerator::~CFGGenerator() {}

CFGNode* CFGGenerator::generateCFG() {
  if (!astRoot) return nullptr;

  unordered_map<ASTNode*, CFGNode*> astToCfgMap;
  CFGNode* rootCFGNode = createCFGNode(astRoot);
  buildCFGRecursively(astRoot, rootCFGNode, astToCfgMap);

  return rootCFGNode;
}

CFGNode* CFGGenerator::createCFGNode(ASTNode* astNode) {
  return new CFGNode(nodeIdCounter++, astNode);
}

void CFGGenerator::buildCFGRecursively(
    ASTNode* astNode, CFGNode* cfgNode,
    unordered_map<ASTNode*, CFGNode*>& astToCfgMap) {
  if (!astNode || astToCfgMap.count(astNode)) return;

  astToCfgMap[astNode] = cfgNode;

  for (auto* child : astNode->children) {
    CFGNode* childCFGNode = createCFGNode(child);
    cfgNode->successors.push_back(childCFGNode);
    buildCFGRecursively(child, childCFGNode, astToCfgMap);
  }
}

void CFGGenerator::printCFG(CFGNode* node, unordered_set<int>& visited) {
  if (!node || visited.count(node->id)) return;

  visited.insert(node->id);
  cout << "CFG Node " << node->id << " -> ";

  for (auto* successor : node->successors) {
    cout << successor->id << " ";
  }
  cout << endl;

  for (auto* successor : node->successors) {
    printCFG(successor, visited);
  }
}