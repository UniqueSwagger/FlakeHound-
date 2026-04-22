#include "static_analyzer.h"

#include <fstream>
#include <iomanip>
#include <sstream>

#include "lexer.h"

using namespace std;

int countOccurrences(const string& source, const vector<string>& needles) {
  int count = 0;

  for (const string& needle : needles) {
    int pos = 0;
    while ((pos = source.find(needle, pos)) != string::npos) {
      count++;
      pos += needle.length();
    }
  }

  return count;
}

StaticAnalyzer::StaticAnalyzer() : riskModelReady(false) {
  initializeRiskModel();
}

StaticAnalysisReport StaticAnalyzer::analyzeSourceCode(
    const string& targetName, const string& sourceCode) {
  StaticAnalysisReport report;
  report.targetName = targetName;

  Lexer lexer(sourceCode);
  vector<Token> tokens = lexer.tokenize();
  report.tokenCount = (int)(tokens.size());

  ASTBuilder builder(tokens);
  ASTNode* astRoot = builder.buildAST();

  int astNodeCount = 0;
  collectAstMetrics(astRoot, nullptr, astRoot, report.features, astNodeCount,
                    report.findings);
  report.astNodeCount = astNodeCount;

  CFGGenerator cfgGenerator(astRoot);
  CFGNode* cfgRoot = cfgGenerator.generateCFG();
  report.cfgNodeCount = cfgGenerator.countReachableNodes(cfgRoot);

  detectSourcePatterns(sourceCode, report.features, report.findings);

  vector<double> modelFeatures = buildModelFeatures(report);
  report.mlRiskScore = riskModel.predictProbability(modelFeatures);
  if (report.mlRiskScore >= 0.70) {
    report.riskCategory = "high_risk";
  } else if (report.mlRiskScore >= 0.40) {
    report.riskCategory = "medium_risk";
  } else {
    report.riskCategory = "low_risk";
  }

  return report;
}

StaticAnalysisReport StaticAnalyzer::analyzeFile(const string& filename) {
  ifstream input(filename);
  stringstream buffer;
  buffer << input.rdbuf();
  return analyzeSourceCode(filename, buffer.str());
}

string StaticAnalyzer::generateTextReport(
    const StaticAnalysisReport& report) const {
  stringstream output;

  output << "FlakeHound++ Static Analysis Report\n";
  output << "Target: " << report.targetName << "\n";
  output << "Tokens: " << report.tokenCount << "\n";
  output << "AST Nodes: " << report.astNodeCount << "\n";
  output << "CFG Nodes: " << report.cfgNodeCount << "\n";
  output << "Predicted Risk: " << fixed << setprecision(3) << report.mlRiskScore
         << " (" << report.riskCategory << ")\n\n";

  output << "Feature Summary\n";
  output << "Random Usage: " << report.features.randomUsageCount << "\n";
  output << "Sleep/Wait Usage: " << report.features.sleepWaitCount << "\n";
  output << "Time Usage: " << report.features.timeUsageCount << "\n";
  output << "Environment Usage: " << report.features.environmentUsageCount
         << "\n";
  output << "Global Variables: " << report.features.globalVariableCount << "\n";
  output << "Thread Usage: " << report.features.threadUsageCount << "\n";
  output << "Null Usage: " << report.features.nullUsageCount << "\n";
  output << "Division By Zero Patterns: " << report.features.divisionByZeroCount
         << "\n";
  output << "Array Indexing: " << report.features.arrayIndexCount << "\n";
  output << "Uninitialized Variables: "
         << report.features.uninitializedVariableCount << "\n";
  output << "Missing Returns: " << report.features.missingReturnCount << "\n";
  output << "Branches: " << report.features.branchCount << "\n";
  output << "Loops: " << report.features.loopCount << "\n";
  output << "Functions: " << report.features.functionCount << "\n";
  output << "Returns: " << report.features.returnCount << "\n\n";

  output << "Findings\n";
  if (report.findings.empty()) {
    output << "No issues detected by the current rule set.\n";
  } else {
    for (int i = 0; i < report.findings.size(); i++) {
      const StaticFinding& finding = report.findings[i];
      output << (i + 1) << ". [" << finding.severity << "] " << finding.category
             << ": " << finding.message << "\n";
      if (!finding.evidence.empty()) {
        output << "   Evidence: " << finding.evidence << "\n";
      }
      if (finding.line > 0) {
        output << "   Line: " << finding.line << "\n";
      }
    }
  }

  return output.str();
}

void StaticAnalyzer::initializeRiskModel() {
  if (riskModelReady) {
    return;
  }

  vector<vector<double>> features = {
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 6, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 12, 0},
      {1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 10, 2},
      {0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 8, 2},
      {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 9, 2},
      {0, 0, 0, 0, 1, 2, 0, 0, 0, 0, 0, 2, 1, 18, 3},
      {0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 7, 4},
      {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 3, 2, 20, 3},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0},
      {1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 2, 1, 22, 5}};
  vector<int> labels = {0, 0, 1, 1, 1, 1, 1, 1, 0, 1};

  riskModel.initialize(features.front().size());
  riskModel.train(features, labels, 600, 0.04, 0.0005);
  riskModelReady = true;
}

void StaticAnalyzer::collectAstMetrics(ASTNode* node, ASTNode* parent,
                                       ASTNode* programRoot,
                                       StaticFeatureVector& features,
                                       int& astNodeCount,
                                       vector<StaticFinding>& findings) const {
  if (!node) {
    return;
  }

  astNodeCount++;

  switch (node->type) {
    case NODE_FUNCTION:
      features.functionCount++;
      if (functionReturnType(node) != "void" &&
          !subtreeContainsType(node, NODE_RETURN)) {
        features.missingReturnCount++;
        addFinding(findings, "code_problem", "high",
                   "Non-void function may exit without returning a value.",
                   node->value, node->line);
      }
      break;

    case NODE_VARIABLE:
      if (parent == programRoot) {
        features.globalVariableCount++;
        addFinding(findings, "flakiness_cause", "medium",
                   "Global mutable state can make tests order-dependent.",
                   node->value, node->line);
      }
      if (node->children.empty() && parent != programRoot) {
        features.uninitializedVariableCount++;
        addFinding(findings, "code_problem", "medium",
                   "Variable declared without an initializer.", node->value,
                   node->line);
      }
      break;

    case NODE_IF:
      features.branchCount++;
      break;

    case NODE_WHILE:
    case NODE_FOR:
      features.loopCount++;
      break;

    case NODE_RETURN:
      features.returnCount++;
      break;

    case NODE_EXPRESSION:
      if ((node->value == "/" || node->value == "%") &&
          node->children.size() >= 2 &&
          node->children[1]->type == NODE_NUMBER &&
          node->children[1]->value == "0") {
        features.divisionByZeroCount++;
        addFinding(findings, "code_problem", "high",
                   "Possible division by zero detected in an expression.",
                   node->value + " 0", node->line);
      }
      break;

    default:
      break;
  }

  for (ASTNode* child : node->children) {
    collectAstMetrics(child, node, programRoot, features, astNodeCount,
                      findings);
  }
}

void StaticAnalyzer::detectSourcePatterns(
    const string& sourceCode, StaticFeatureVector& features,
    vector<StaticFinding>& findings) const {
  features.randomUsageCount += countOccurrences(
      sourceCode, {"rand(", "random_device", "mt19937", "uniform_"});
  if (features.randomUsageCount > 0) {
    addFinding(findings, "flakiness_cause", "high",
               "Randomness detected without any stability guarantees.",
               "rand()/random_device/mt19937",
               findLineNumber(sourceCode, {"rand(", "random_device", "mt19937",
                                           "uniform_"}));
  }

  features.sleepWaitCount +=
      countOccurrences(sourceCode, {"sleep_for", "sleep_until", "usleep",
                                    "wait_for", "wait_until", "nanosleep"});
  if (features.sleepWaitCount > 0) {
    addFinding(
        findings, "flakiness_cause", "high",
        "Sleep or wait-based timing logic can make tests flaky.",
        "sleep_for/wait_for",
        findLineNumber(sourceCode, {"sleep_for", "sleep_until", "usleep",
                                    "wait_for", "wait_until", "nanosleep"}));
  }

  features.timeUsageCount += countOccurrences(
      sourceCode, {"time(", "chrono::", "system_clock", "steady_clock",
                   "high_resolution_clock", "now()"});
  if (features.timeUsageCount > 0) {
    addFinding(findings, "flakiness_cause", "high",
               "Time-dependent logic detected.", "time()/chrono usage",
               findLineNumber(sourceCode, {"time(", "chrono::", "system_clock",
                                           "steady_clock",
                                           "high_resolution_clock", "now()"}));
  }

  features.environmentUsageCount +=
      countOccurrences(sourceCode, {"getenv(", "setenv(", "putenv("});
  if (features.environmentUsageCount > 0) {
    addFinding(findings, "flakiness_cause", "medium",
               "Environment-dependent behavior detected.", "getenv/setenv",
               findLineNumber(sourceCode, {"getenv(", "setenv(", "putenv("}));
  }

  features.threadUsageCount +=
      countOccurrences(sourceCode, {"std::thread", "pthread_", "std::async",
                                    "condition_variable", "mutex", "atomic<",
                                    "std::this_thread"});
  if (features.threadUsageCount > 0) {
    addFinding(
        findings, "flakiness_cause", "high",
        "Concurrency primitives detected; race conditions may affect tests.",
        "thread/mutex/atomic",
        findLineNumber(sourceCode, {"std::thread", "pthread_", "std::async",
                                    "condition_variable", "mutex", "atomic<",
                                    "std::this_thread"}));
  }

  features.nullUsageCount += countOccurrences(sourceCode, {"nullptr", "NULL"});
  if (features.nullUsageCount > 0) {
    addFinding(findings, "code_problem", "medium",
               "Null pointer usage should be reviewed for dereference safety.",
               "nullptr/NULL", findLineNumber(sourceCode, "nullptr"));
  }

  features.divisionByZeroCount +=
      countOccurrences(sourceCode, {"/ 0", "/0", "% 0", "%0"});
  if (features.divisionByZeroCount > 0) {
    addFinding(findings, "code_problem", "high",
               "Literal division by zero pattern found in source text.", "/ 0",
               findLineNumber(sourceCode, "/ 0"));
  }

  features.arrayIndexCount += countOccurrences(sourceCode, {"["});
  if (features.arrayIndexCount > 0) {
    addFinding(findings, "code_problem", "low",
               "Array indexing detected; bounds checks should be reviewed.",
               "[] usage", findLineNumber(sourceCode, "["));
  }
}

vector<double> StaticAnalyzer::buildModelFeatures(
    const StaticAnalysisReport& report) const {
  return {(double)(report.features.randomUsageCount),
          (double)(report.features.sleepWaitCount),
          (double)(report.features.timeUsageCount),
          (double)(report.features.environmentUsageCount),
          (double)(report.features.globalVariableCount),
          (double)(report.features.threadUsageCount),
          (double)(report.features.nullUsageCount),
          (double)(report.features.divisionByZeroCount),
          (double)(report.features.arrayIndexCount),
          (double)(report.features.uninitializedVariableCount),
          (double)(report.features.missingReturnCount),
          (double)(report.features.branchCount),
          (double)(report.features.loopCount),
          (double)(report.cfgNodeCount),
          (double)(report.findings.size())};
}

void StaticAnalyzer::addFinding(vector<StaticFinding>& findings,
                                const string& category, const string& severity,
                                const string& message, const string& evidence,
                                int line) const {
  for (const StaticFinding& existing : findings) {
    if (existing.message == message && existing.evidence == evidence &&
        existing.line == line) {
      return;
    }
  }

  findings.push_back({category, severity, message, evidence, line});
}

int StaticAnalyzer::findLineNumber(const string& sourceCode,
                                   const string& needle) const {
  if (needle.empty()) {
    return 0;
  }

  int pos = sourceCode.find(needle);
  if (pos == string::npos) {
    return 0;
  }

  int line = 1;
  for (int i = 0; i < pos; i++) {
    if (sourceCode[i] == '\n') {
      line++;
    }
  }
  return line;
}

int StaticAnalyzer::findLineNumber(const string& sourceCode,
                                   const vector<string>& needles) const {
  for (const string& needle : needles) {
    int line = findLineNumber(sourceCode, needle);
    if (line > 0) {
      return line;
    }
  }
  return 0;
}

bool StaticAnalyzer::subtreeContainsType(ASTNode* node, int type) const {
  if (!node) {
    return false;
  }

  if (node->type == type) {
    return true;
  }

  for (ASTNode* child : node->children) {
    if (subtreeContainsType(child, type)) {
      return true;
    }
  }

  return false;
}

string StaticAnalyzer::functionReturnType(ASTNode* functionNode) const {
  if (!functionNode) {
    return "";
  }

  int separator = functionNode->value.find(' ');
  if (separator == string::npos) {
    return functionNode->value;
  }
  return functionNode->value.substr(0, separator);
}
