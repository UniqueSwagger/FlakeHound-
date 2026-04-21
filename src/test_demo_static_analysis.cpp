#include <filesystem>
#include <iostream>
#include <string>

#include "static_analyzer.h"

using namespace std;

namespace {

filesystem::path findDemoSource(int argc, char* argv[]) {
  if (argc >= 2) {
    filesystem::path requested = filesystem::absolute(argv[1]);
    if (filesystem::exists(requested)) {
      return requested;
    }
  }

  const filesystem::path candidates[] = {
      filesystem::path("demo") / "demo_tests.cpp",
      filesystem::path(__FILE__).parent_path().parent_path() / "demo" /
          "demo_tests.cpp"};

  for (const filesystem::path& candidate : candidates) {
    filesystem::path absolutePath = filesystem::absolute(candidate);
    if (filesystem::exists(absolutePath)) {
      return absolutePath;
    }
  }

  return {};
}

}  // namespace

int main(int argc, char* argv[]) {
  filesystem::path demoSource = findDemoSource(argc, argv);
  if (demoSource.empty()) {
    cerr << "Could not find demo_tests.cpp for static analysis.\n";
    return 1;
  }

  StaticAnalyzer analyzer;
  StaticAnalysisReport report = analyzer.analyzeFile(demoSource.string());

  if (report.features.randomUsageCount == 0) {
    cerr << "Expected randomness detection in demo source.\n";
    return 1;
  }

  if (report.features.timeUsageCount == 0) {
    cerr << "Expected time-based detection in demo source.\n";
    return 1;
  }

  if (report.mlRiskScore <= 0.0) {
    cerr << "Expected a non-zero static risk score.\n";
    return 1;
  }

  cout << analyzer.generateTextReport(report);
  cout << "\nDemo static-analysis smoke test passed.\n";
  return 0;
}
