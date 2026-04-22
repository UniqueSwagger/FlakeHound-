#include "cfg_generator.h"

#include <iostream>

#include "ast_builder.h"

using namespace std;

CFGGenerator::CFGGenerator(ASTNode* astRoot)
    : astRoot(astRoot), nodeIdCounter(0) {}

CFGGenerator::~CFGGenerator() {
  for (CFGNode* node : allNodes) {
    delete node;
  }
}

CFGNode* CFGGenerator::generateCFG() {
  if (!astRoot) {
    return nullptr;
  }

  CFGFragment fragment = buildFragment(astRoot);
  return fragment.entry;
}

CFGNode* CFGGenerator::createCFGNode(ASTNode* astNode, const string& label) {
  CFGNode* node = new CFGNode(nodeIdCounter++, astNode, label);
  allNodes.push_back(node);
  return node;
}

string CFGGenerator::describeNode(ASTNode* astNode) const {
  if (!astNode) {
    return "synthetic";
  }

  if (!astNode->value.empty()) {
    return astNode->value;
  }

  switch (astNode->type) {
    case NODE_PROGRAM:
      return "program";
    case NODE_FUNCTION:
      return "function";
    case NODE_VARIABLE:
      return "variable";
    case NODE_IF:
      return "if";
    case NODE_WHILE:
      return "while";
    case NODE_FOR:
      return "for";
    case NODE_RETURN:
      return "return";
    case NODE_BLOCK:
      return "block";
    default:
      return "node";
  }
}

void CFGGenerator::connectToEntry(const vector<CFGNode*>& from, CFGNode* to) {
  if (!to) {
    return;
  }

  for (CFGNode* node : from) {
    if (node) {
      node->successors.push_back(to);
    }
  }
}

CFGGenerator::CFGFragment CFGGenerator::buildSequence(
    const vector<ASTNode*>& nodes) {
  CFGFragment sequence{nullptr, {}};

  for (ASTNode* child : nodes) {
    CFGFragment part = buildFragment(child);
    if (!part.entry) {
      continue;
    }

    if (!sequence.entry) {
      sequence.entry = part.entry;
    } else {
      connectToEntry(sequence.exits, part.entry);
    }

    sequence.exits = part.exits;
  }

  if (!sequence.entry) {
    CFGNode* emptyNode = createCFGNode(nullptr, "empty");
    sequence.entry = emptyNode;
    sequence.exits.push_back(emptyNode);
  }

  return sequence;
}

CFGGenerator::CFGFragment CFGGenerator::buildFragment(ASTNode* astNode) {
  if (!astNode) {
    return {nullptr, {}};
  }

  switch (astNode->type) {
    case NODE_PROGRAM:
    case NODE_BLOCK:
      return buildSequence(astNode->children);

    case NODE_FUNCTION: {
      CFGNode* functionNode = createCFGNode(astNode, describeNode(astNode));
      vector<ASTNode*> bodyNodes;
      for (ASTNode* child : astNode->children) {
        if (child->type == NODE_BLOCK) {
          bodyNodes.push_back(child);
        }
      }

      if (bodyNodes.empty()) {
        return {functionNode, {functionNode}};
      }

      CFGFragment body = buildSequence(bodyNodes);
      functionNode->successors.push_back(body.entry);
      return {functionNode, body.exits};
    }

    case NODE_IF: {
      CFGNode* conditionNode = createCFGNode(astNode, describeNode(astNode));
      CFGNode* mergeNode = createCFGNode(nullptr, "if_merge");

      CFGFragment thenBranch = astNode->children.size() > 1
                                   ? buildFragment(astNode->children[1])
                                   : CFGFragment{nullptr, {}};
      CFGFragment elseBranch = astNode->children.size() > 2
                                   ? buildFragment(astNode->children[2])
                                   : CFGFragment{nullptr, {}};

      if (thenBranch.entry) {
        conditionNode->successors.push_back(thenBranch.entry);
        connectToEntry(thenBranch.exits, mergeNode);
      } else {
        conditionNode->successors.push_back(mergeNode);
      }

      if (elseBranch.entry) {
        conditionNode->successors.push_back(elseBranch.entry);
        connectToEntry(elseBranch.exits, mergeNode);
      } else {
        conditionNode->successors.push_back(mergeNode);
      }

      return {conditionNode, {mergeNode}};
    }

    case NODE_WHILE: {
      CFGNode* conditionNode = createCFGNode(astNode, describeNode(astNode));
      CFGNode* afterLoopNode = createCFGNode(nullptr, "while_exit");
      CFGFragment body = astNode->children.size() > 1
                             ? buildFragment(astNode->children[1])
                             : CFGFragment{nullptr, {}};

      if (body.entry) {
        conditionNode->successors.push_back(body.entry);
        connectToEntry(body.exits, conditionNode);
      }

      conditionNode->successors.push_back(afterLoopNode);
      return {conditionNode, {afterLoopNode}};
    }

    case NODE_FOR: {
      CFGFragment init = !astNode->children.empty()
                             ? buildFragment(astNode->children[0])
                             : CFGFragment{nullptr, {}};
      CFGNode* conditionNode = createCFGNode(astNode, describeNode(astNode));
      CFGFragment update = astNode->children.size() > 2
                               ? buildFragment(astNode->children[2])
                               : CFGFragment{nullptr, {}};
      CFGFragment body = astNode->children.size() > 3
                             ? buildFragment(astNode->children[3])
                             : CFGFragment{nullptr, {}};
      CFGNode* afterLoopNode = createCFGNode(nullptr, "for_exit");

      if (init.entry) {
        connectToEntry(init.exits, conditionNode);
      }

      if (body.entry) {
        conditionNode->successors.push_back(body.entry);
        if (update.entry) {
          connectToEntry(body.exits, update.entry);
          connectToEntry(update.exits, conditionNode);
        } else {
          connectToEntry(body.exits, conditionNode);
        }
      }

      conditionNode->successors.push_back(afterLoopNode);

      if (init.entry) {
        return {init.entry, {afterLoopNode}};
      }
      return {conditionNode, {afterLoopNode}};
    }

    case NODE_RETURN: {
      CFGNode* returnNode = createCFGNode(astNode, describeNode(astNode));
      return {returnNode, {}};
    }

    default: {
      CFGNode* node = createCFGNode(astNode, describeNode(astNode));
      return {node, {node}};
    }
  }
}

void CFGGenerator::printCFG(CFGNode* node, unordered_set<int>& visited) {
  if (!node || visited.count(node->id)) {
    return;
  }

  visited.insert(node->id);
  cout << "CFG Node " << node->id << " [" << node->label << "] -> ";

  for (CFGNode* successor : node->successors) {
    cout << successor->id << " ";
  }
  cout << endl;

  for (CFGNode* successor : node->successors) {
    printCFG(successor, visited);
  }
}

int CFGGenerator::countReachableNodes(CFGNode* node) {
  unordered_set<int> visited;
  return countReachableNodesRecursively(node, visited);
}

int CFGGenerator::countReachableNodesRecursively(CFGNode* node,
                                                 unordered_set<int>& visited) {
  if (!node || visited.count(node->id)) {
    return 0;
  }

  visited.insert(node->id);
  int count = 1;
  for (CFGNode* successor : node->successors) {
    count += countReachableNodesRecursively(successor, visited);
  }
  return count;
}
