#include <iostream>
#include <string>

#include "static_analyzer.h"

using namespace std;

int main() {
  const string sourceCode = R"(
int shared_counter;

int flaky_helper() {
  int localValue;
  int randomValue = rand();
  if (time(nullptr) % 2 == 0) {
    sleep_for(10);
  }
  return randomValue + localValue;
}
)";

  StaticAnalyzer analyzer;
  StaticAnalysisReport report =
      analyzer.analyzeSourceCode("inline_flaky_sample.cpp", sourceCode);

  if (report.features.randomUsageCount == 0) {
    cerr << "Expected random usage detection\n";
    return 1;
  }

  if (report.features.timeUsageCount == 0) {
    cerr << "Expected time usage detection\n";
    return 1;
  }

  if (report.features.uninitializedVariableCount == 0) {
    cerr << "Expected uninitialized variable detection\n";
    return 1;
  }

  if (report.findings.empty()) {
    cerr << "Expected at least one static-analysis finding\n";
    return 1;
  }

  if (report.mlRiskScore <= 0.0) {
    cerr << "Expected a non-zero predicted risk score\n";
    return 1;
  }

  cout << analyzer.generateTextReport(report);
  cout << "\nStatic analyzer smoke test passed.\n";
  return 0;
}
