#ifndef CFG_GENERATOR_H
#define CFG_GENERATOR_H

#include <unordered_map>
#include <unordered_set>
#include <vector>

class ASTNode;

class CFGNode {
 public:
  int id;
  std::vector<CFGNode*> successors;
  ASTNode* astNode;

  CFGNode(int nodeId, ASTNode* node) : id(nodeId), astNode(node) {}
};

class CFGGenerator {
 public:
  CFGGenerator(ASTNode* astRoot);
  ~CFGGenerator();

  CFGNode* generateCFG();
  void printCFG(CFGNode* node, std::unordered_set<int>& visited);

 private:
  ASTNode* astRoot;
  int nodeIdCounter;

  CFGNode* createCFGNode(ASTNode* astNode);
  void buildCFGRecursively(ASTNode* astNode, CFGNode* cfgNode,
                           std::unordered_map<ASTNode*, CFGNode*>& astToCfgMap);
};

#endif  // CFG_GENERATOR_H