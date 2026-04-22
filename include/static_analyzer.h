#ifndef STATIC_ANALYZER_H
#define STATIC_ANALYZER_H

#include <map>
#include <string>
#include <vector>

#include "ast_builder.h"
#include "cfg_generator.h"
#include "logistic_regression.h"

using namespace std;

struct StaticFinding {
  string category;
  string severity;
  string message;
  string evidence;
  int line;
};

struct StaticFeatureVector {
  int randomUsageCount = 0;
  int sleepWaitCount = 0;
  int timeUsageCount = 0;
  int environmentUsageCount = 0;
  int globalVariableCount = 0;
  int threadUsageCount = 0;
  int nullUsageCount = 0;
  int divisionByZeroCount = 0;
  int arrayIndexCount = 0;
  int uninitializedVariableCount = 0;
  int missingReturnCount = 0;
  int branchCount = 0;
  int loopCount = 0;
  int returnCount = 0;
  int functionCount = 0;
};

struct StaticAnalysisReport {
  string targetName;
  int tokenCount = 0;
  int astNodeCount = 0;
  int cfgNodeCount = 0;
  StaticFeatureVector features;
  vector<StaticFinding> findings;
  double mlRiskScore = 0.0;
  string riskCategory;
};

class StaticAnalyzer {
 public:
  StaticAnalyzer();

  StaticAnalysisReport analyzeSourceCode(const string& targetName,
                                         const string& sourceCode);
  StaticAnalysisReport analyzeFile(const string& filename);
  string generateTextReport(const StaticAnalysisReport& report) const;

 private:
  LogisticRegressionSGD riskModel;
  bool riskModelReady;

  void initializeRiskModel();
  void collectAstMetrics(ASTNode* node, ASTNode* parent, ASTNode* programRoot,
                         StaticFeatureVector& features, int& astNodeCount,
                         vector<StaticFinding>& findings) const;
  void detectSourcePatterns(const string& sourceCode,
                            StaticFeatureVector& features,
                            vector<StaticFinding>& findings) const;
  vector<double> buildModelFeatures(const StaticAnalysisReport& report) const;
  void addFinding(vector<StaticFinding>& findings, const string& category,
                  const string& severity, const string& message,
                  const string& evidence, int line) const;
  int findLineNumber(const string& sourceCode, const string& needle) const;
  int findLineNumber(const string& sourceCode,
                     const vector<string>& needles) const;
  bool subtreeContainsType(ASTNode* node, int type) const;
  string functionReturnType(ASTNode* functionNode) const;
};

#endif
