#ifndef CFG_GENERATOR_H
#define CFG_GENERATOR_H
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class ASTNode;

class CFGNode {
 public:
  int id;
  std::string label;
  std::vector<CFGNode*> successors;
  ASTNode* astNode;

  CFGNode(int nodeId, ASTNode* node, const std::string& nodeLabel = "") {
    id = nodeId;
    label = nodeLabel;
    astNode = node;
  }
};

class CFGGenerator {
 public:
  CFGGenerator(ASTNode* astRoot);
  ~CFGGenerator();

  CFGNode* generateCFG();
  void printCFG(CFGNode* node, std::unordered_set<int>& visited);
  int countReachableNodes(CFGNode* node);

 private:
  struct CFGFragment {
    CFGNode* entry;
    std::vector<CFGNode*> exits;
  };

  ASTNode* astRoot;
  int nodeIdCounter;
  std::vector<CFGNode*> allNodes;

  CFGNode* createCFGNode(ASTNode* astNode, const std::string& label = "");
  std::string describeNode(ASTNode* astNode) const;
  CFGFragment buildFragment(ASTNode* astNode);
  CFGFragment buildSequence(const std::vector<ASTNode*>& nodes);
  void connectToEntry(const std::vector<CFGNode*>& from, CFGNode* to);
  int countReachableNodesRecursively(CFGNode* node,
                                     std::unordered_set<int>& visited);
};

#endif  // CFG_GENERATOR_H
