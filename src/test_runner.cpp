#include "test_runner.h"

#include <cstdlib>
#include <iostream>
#include <sstream>

using namespace std;

TestRunner::TestRunner(string exePath) : executable(exePath) {}

TestRunner::~TestRunner() {}

vector<RunResult> TestRunner::runMultipleTimes(int times) {
  allResults.clear();

  cout << "Running tests " << times << " times...\n\n";

  for (int i = 1; i <= times; i++) {
    cout << "Run #" << i << " ... ";
    RunResult result = runOnce(i);
    allResults.push_back(result);
    cout << "Passed: " << result.passedTests
         << ", Failed: " << result.failedTests << "\n";
  }

  return allResults;
}

RunResult TestRunner::runOnce(int runNumber) {
  RunResult result;
  result.runNumber = runNumber;
  result.totalTests = 0;
  result.passedTests = 0;
  result.failedTests = 0;

  string tempFile = "temp_run_" + to_string(runNumber) + ".txt";
  string command = executable + " > " + tempFile + " 2>&1";

  system(command.c_str());

  FILE* file = fopen(tempFile.c_str(), "r");
  if (file) {
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), file)) {
      result.output += buffer;
    }
    fclose(file);
  }

  parseOutput(result);

  remove(tempFile.c_str());

  return result;
}

void TestRunner::parseOutput(RunResult& result) {
  string output = result.output;

  size_t pos = 0;
  while ((pos = output.find("PASS:", pos)) != string::npos) {
    result.passedTests++;
    pos += 5;
  }

  pos = 0;
  while ((pos = output.find("FAIL:", pos)) != string::npos) {
    result.failedTests++;
    pos += 5;
  }

  result.totalTests = result.passedTests + result.failedTests;
}

map<string, int> TestRunner::getStats() {
  map<string, int> stats;

  int totalRuns = allResults.size();
  int totalPassed = 0;
  int totalFailed = 0;
  int totalTests = 0;

  for (auto& result : allResults) {
    totalPassed += result.passedTests;
    totalFailed += result.failedTests;
    totalTests += result.totalTests;
  }

  stats["runs"] = totalRuns;
  stats["total_passed"] = totalPassed;
  stats["total_failed"] = totalFailed;
  stats["total_tests"] = totalTests;

  return stats;
}
