#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "flakliness_calculator.h"
#include "report_generator.h"
#include "test_runner.h"

using namespace std;

namespace {

string quotePath(const filesystem::path& path) {
  return "\"" + path.string() + "\"";
}

filesystem::path findExecutablePath(int argc, char* argv[]) {
  vector<filesystem::path> candidates;

  if (argc >= 2) {
    candidates.push_back(argv[1]);
  }

  candidates.push_back("demo_tests");
  candidates.push_back("demo_tests.exe");
  candidates.push_back(filesystem::path("demo") / "demo_tests");
  candidates.push_back(filesystem::path("demo") / "demo_tests.exe");
  candidates.push_back(filesystem::path("build") / "bin" / "demo_tests");
  candidates.push_back(filesystem::path("build") / "bin" / "demo_tests.exe");

  for (const filesystem::path& candidate : candidates) {
    if (candidate.empty()) {
      continue;
    }

    filesystem::path absolutePath = filesystem::absolute(candidate);
    if (filesystem::exists(absolutePath)) {
      return absolutePath;
    }
  }

  return {};
}

int parseRuns(int argc, char* argv[]) {
  if (argc >= 3) {
    int requestedRuns = atoi(argv[2]);
    if (requestedRuns > 0) {
      return requestedRuns;
    }
  }

  return 10;
}

string reportPath(int argc, char* argv[]) {
  if (argc >= 4) {
    return argv[3];
  }

  return (filesystem::path("reports") / "flakiness_report.txt").string();
}

string extractTestName(const string& line, const string& prefix) {
  if (line.rfind(prefix, 0) != 0) {
    return "";
  }

  string name = line.substr(prefix.size());
  size_t detailPos = name.find(" (");
  if (detailPos != string::npos) {
    name = name.substr(0, detailPos);
  }

  return name;
}

map<string, vector<bool>> collectTestResults(const vector<RunResult>& results) {
  map<string, vector<bool>> testResults;

  for (const RunResult& result : results) {
    size_t pos = 0;
    while (pos < result.output.length()) {
      size_t newlinePos = result.output.find('\n', pos);
      if (newlinePos == string::npos) {
        newlinePos = result.output.length();
      }

      string line = result.output.substr(pos, newlinePos - pos);
      string testName = extractTestName(line, "PASS: ");
      bool passed = true;

      if (testName.empty()) {
        testName = extractTestName(line, "FAIL: ");
        passed = false;
      }

      if (!testName.empty()) {
        testResults[testName].push_back(passed);
      }

      pos = newlinePos + 1;
    }
  }

  return testResults;
}

}  // namespace

int main(int argc, char* argv[]) {
  cout << "FlakeHound++ - Test Runner Demo\n";

  filesystem::path executablePath = findExecutablePath(argc, argv);
  if (executablePath.empty()) {
    cerr << "Error: could not find a demo test executable.\n";
    cerr << "Pass the executable path as the first argument.\n";
    return 1;
  }

  int runs = parseRuns(argc, argv);
  TestRunner runner(quotePath(executablePath));
  vector<RunResult> results = runner.runMultipleTimes(runs);
  map<string, int> stats = runner.getStats();

  cout << "Overall Results\n";
  cout << "Total Runs: " << stats["runs"] << "\n";
  cout << "Total Tests Passed: " << stats["total_passed"] << "\n";
  cout << "Total Tests Failed: " << stats["total_failed"] << "\n";
  cout << "Total Tests: " << stats["total_tests"] << "\n\n";

  if (stats["total_tests"] == 0) {
    cerr << "Error: the test runner did not capture any PASS/FAIL output.\n";
    return 1;
  }

  map<string, vector<bool>> testResults = collectTestResults(results);
  if (testResults.empty()) {
    cerr << "Error: no per-test results were extracted from the test output.\n";
    return 1;
  }

  FlakinessCalculator calculator;
  vector<FlakinessScore> scores =
      calculator.calculateForAllTests(testResults);

  cout << "Flakiness Analysis\n\n";
  for (const FlakinessScore& score : scores) {
    cout << score.testName << ": PASSED " << score.passes << "/"
         << score.totalRuns << " times (Category: " << score.category << ")\n";
  }
  cout << "\n";

  ReportGenerator reportGen;
  reportGen.generateReport(reportPath(argc, argv), scores);

  cout << "FlakeHound++ Analysis Complete!\n";

  return 0;
}
